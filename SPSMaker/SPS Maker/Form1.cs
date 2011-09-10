using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace SPS_Maker
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void button_OpenNDS_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog fbd = new FolderBrowserDialog();
            if (fbd.ShowDialog() == DialogResult.OK)
            {
                string[] filePaths = Directory.GetFiles(fbd.SelectedPath, "*.nds", SearchOption.AllDirectories);

                if (!Directory.Exists(Path.GetPathRoot(filePaths[0]) + "data\\NDS Music Player\\"))
                {
                    System.IO.Directory.CreateDirectory(Path.GetPathRoot(filePaths[0]) + "data\\NDS Music Player\\");
                }
                else
                {
                    string[] filePaths2 = Directory.GetFiles(Path.GetPathRoot(filePaths[0]) + "data\\NDS Music Player\\", "*.sps", SearchOption.AllDirectories);

                    for (int i = 0; i < filePaths2.Length; i++)
                    {
                        File.Delete(filePaths2[i]);
                    }
                }

                progressBar1.Value = 0;
                progressBar1.Maximum = filePaths.Length;
                for (int i = 0; i < filePaths.Length; i++)
                {
                    FileStream fs = File.OpenRead(filePaths[i]);
                    BinaryReader br = new BinaryReader(fs);

                    String GameTitle = System.Text.Encoding.UTF8.GetString(br.ReadBytes(12));
                    String TempString = "";

                    UInt32 LastChar = (uint)GameTitle.IndexOf("\0");
                    if (LastChar != 0xFFFFFFFF)
                    {
                        GameTitle = GameTitle.Remove((int)LastChar);
                    }

                    br.BaseStream.Position = 0x48;
                    UInt32 FATOffset = BitConverter.ToUInt32(br.ReadBytes(4), 0);
                    UInt32 FATSize = BitConverter.ToUInt32(br.ReadBytes(4), 0);

                    MemoryStream FAT = new MemoryStream();

                    br.BaseStream.Position = FATOffset;
                    FAT.Write(br.ReadBytes((int)FATSize), 0, (int)FATSize);

                    FAT.Position = 0;
                    UInt32[] SDATOffset = new UInt32[255];
                    UInt32 SDATCount = 0;
                    Byte[] HeaderBytes = new Byte[4];

                    while (FAT.Position < FATSize)
                    {
                        FAT.Read(HeaderBytes, 0, 4);
                        SDATOffset[SDATCount] = BitConverter.ToUInt32(HeaderBytes, 0);

                        br.BaseStream.Position = SDATOffset[SDATCount];
                        br.Read(HeaderBytes, 0, 4);

                        if (System.Text.Encoding.UTF8.GetString(HeaderBytes) == "SDAT")
                        {
                            br.Read(HeaderBytes, 0, 4);
                            if (BitConverter.ToUInt32(HeaderBytes, 0) == 0x0100FEFF)
                            {
                                SDATCount++;    //Prevent any false positives.  SDAT always begins with "SDAT",0xFF,0xFE,0x00,0x01.
                            }
                        }
                        FAT.Position += 4;
                    }

                    if (SDATCount > 0)
                    {
                        UInt32 SSEQCount = 0;
                        
                        TempString = GameTitle;

                        TempString = TempString.Replace('<', '_');
                        TempString = TempString.Replace(':', '_');
                        TempString = TempString.Replace('"', '_');
                        TempString = TempString.Replace('/', '_');
                        TempString = TempString.Replace('\\', '_');
                        TempString = TempString.Replace('|', '_');
                        TempString = TempString.Replace('?', '_');
                        TempString = TempString.Replace('*', '_');
                        TempString = TempString.Replace('>', '_');

                        FileStream fs2 = File.Create(Path.GetPathRoot(filePaths[i]) + "data\\NDS Music Player\\" + TempString + ".sps");
                        BinaryWriter bw = new BinaryWriter(fs2);

                        TempString = filePaths[i].Replace("\\", "/"); ;
                        bw.BaseStream.Position = 0xC;
                        bw.BaseStream.WriteByte(Convert.ToByte(TempString.Length - 2));
                        bw.BaseStream.Write(Encoding.ASCII.GetBytes(TempString), 2, filePaths[i].Length - 2);

                        UInt32 NameOffset = (uint)bw.BaseStream.Position;
                        bw.BaseStream.Position = 0x4;
                        bw.BaseStream.Write(BitConverter.GetBytes(NameOffset), 0, 4);

                        UInt32[] SDATSSEQCount = new UInt32[SDATCount];

                        for (int j = 0; j < SDATCount; j++) //Sum up the SSEQCount from ALL SDATs in the rom
                        {
                            br.BaseStream.Position = SDATOffset[j] + 0x10;
                            UInt32 SYMBOffset = SDATOffset[j] + BitConverter.ToUInt32(br.ReadBytes(4), 0);

                            br.BaseStream.Position = SYMBOffset;
                            br.Read(HeaderBytes, 0, 4);

                            if (System.Text.Encoding.UTF8.GetString(HeaderBytes) != "SYMB")
                            {
                                br.BaseStream.Position = SDATOffset[j] + 0x18;
                                UInt32 InfoOffset = BitConverter.ToUInt32(br.ReadBytes(4), 0) + SDATOffset[j];

                                br.BaseStream.Position = InfoOffset + 0x8;
                                UInt32 InfoSSEQRecOffset = BitConverter.ToUInt32(br.ReadBytes(4), 0) + InfoOffset;

                                br.BaseStream.Position = InfoSSEQRecOffset;
                                UInt32 TempSSEQCount = BitConverter.ToUInt32(br.ReadBytes(4), 0);
                                SSEQCount += TempSSEQCount;
                                SDATSSEQCount[j] = TempSSEQCount;
                            }
                            else
                            {
                                br.BaseStream.Position = SYMBOffset + 0x8;
                                UInt32 SSEQRecOffset = SYMBOffset + BitConverter.ToUInt32(br.ReadBytes(4), 0);

                                br.BaseStream.Position = SSEQRecOffset;
                                UInt32 TempSSEQCount = BitConverter.ToUInt32(br.ReadBytes(4), 0);
                                SSEQCount += TempSSEQCount;
                                SDATSSEQCount[j] = TempSSEQCount;
                            }
                        }

                        if (SSEQCount == 0) //And if in ALL of the SDATS, there was absolutely no sequences, delete this SPS.
                        {
                            bw.Close();
                            TempString = GameTitle;
                            File.Delete(Path.GetPathRoot(filePaths[i]) + "data\\NDS Music Player\\" + TempString + ".sps");
                            continue;
                        }

                        bw.BaseStream.Position = 0x0;
                        bw.BaseStream.Write(BitConverter.GetBytes(SSEQCount), 0, 4);
                        bw.BaseStream.Position = NameOffset;

                        for (int j = 0; j < SDATCount; j++) //Write the STR List as found in ALL SDATs.
                        {
                            SSEQCount = SDATSSEQCount[j];
                            if (SSEQCount == 0)
                                continue;
                            br.BaseStream.Position = SDATOffset[j] + 0x10;
                            UInt32 SYMBOffset = SDATOffset[j] + BitConverter.ToUInt32(br.ReadBytes(4), 0);

                            br.BaseStream.Position = SYMBOffset;
                            br.Read(HeaderBytes, 0, 4);

                            if (System.Text.Encoding.UTF8.GetString(HeaderBytes) != "SYMB")
                            {

                                UInt32 num;
                                String[] SSEQList = new String[SSEQCount];
                                for (int k = 0; k < SSEQCount; k++)
                                {
                                    num = BitConverter.ToUInt32(br.ReadBytes(4), 0);
                                    if (num != 0)
                                    {
                                        SSEQList[k] = "SSEQ_" + j.ToString("000") + "_" + k.ToString("0000");
                                    }
                                    else
                                    {
                                        SSEQList[k] = "<no entry>";
                                    }

                                    bw.BaseStream.WriteByte(Convert.ToByte(SSEQList[k].Length));
                                    bw.BaseStream.Write(Encoding.ASCII.GetBytes(SSEQList[k]), 0, SSEQList[k].Length);
                                }
                            }
                            else
                            {
                                br.BaseStream.Position = SYMBOffset + 0x8;
                                UInt32 SSEQRecOffset = SYMBOffset + BitConverter.ToUInt32(br.ReadBytes(4), 0);

                                br.BaseStream.Position = SSEQRecOffset;
                                BitConverter.ToUInt32(br.ReadBytes(4), 0);
                                

                                UInt32 num;
                                UInt32[] SSEQStringOffsets = new UInt32[SSEQCount];
                                for (int k = 0; k < SSEQCount; k++)
                                {
                                    num = BitConverter.ToUInt32(br.ReadBytes(4), 0);
                                    if (num != 0)
                                    {
                                        SSEQStringOffsets[k] = SYMBOffset + num;
                                    }
                                }

                                String[] SSEQList = new String[SSEQCount];

                                br.BaseStream.Position = SSEQStringOffsets[0];

                                Byte ReadByte = 0xFF;
                                for (int k = 0; k < SSEQCount; k++)
                                {

                                    if (SSEQStringOffsets[k] != 0)
                                    {
                                        br.BaseStream.Position = SSEQStringOffsets[k];
                                        ReadByte = 0xFF;
                                        while (ReadByte != 0)
                                        {
                                            ReadByte = br.ReadByte();
                                            if (ReadByte != 0)
                                            {
                                                SSEQList[k] += Convert.ToChar(ReadByte);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        SSEQList[k] = "<no entry>";
                                    }

                                    if (SSEQList[k] == null)
                                    {
                                        SSEQList[k] = "SSEQ_" + j.ToString("000") + "_" + k.ToString("0000");
                                    }
                                    bw.BaseStream.WriteByte(Convert.ToByte(SSEQList[k].Length));
                                    bw.BaseStream.Write(Encoding.ASCII.GetBytes(SSEQList[k]), 0, SSEQList[k].Length);
                                }
                            }
                        }

                        UInt32 SSEQDataOffset = (uint)bw.BaseStream.Position;
                        bw.BaseStream.Position = 0x8;
                        bw.BaseStream.Write(BitConverter.GetBytes(SSEQDataOffset), 0, 4);
                        bw.BaseStream.Position = SSEQDataOffset;

                        for (int j = 0; j < SDATCount; j++) //Now deal with ALL of the offsets.
                        {
                            SSEQCount = SDATSSEQCount[j];
                            for (int k = 0; k < SSEQCount; k++)
                            {
                                UInt32 InfoOffset = 0;
                                UInt32 SDATFatOffset = 0;
                                UInt32 InfoSSEQRecOffset = 0;
                                UInt32 InfoBANKRecOffset = 0;
                                UInt32 InfoWAVEARCRecOffset = 0;
                                UInt32 BANK = 0;
                                UInt32 WAVEARC1 = 0;
                                UInt32 WAVEARC2 = 0;
                                UInt32 WAVEARC3 = 0;
                                UInt32 WAVEARC4 = 0;
                                UInt32 SSEQID = 0;
                                UInt32 BANKID = 0;
                                UInt32 WAVEARC1ID = 0;
                                UInt32 WAVEARC2ID = 0;
                                UInt32 WAVEARC3ID = 0;
                                UInt32 WAVEARC4ID = 0;
                                UInt32 SSEQIDOffset = 0;
                                UInt32 BANKIDOffset = 0;
                                UInt32 WAVEARC1IDOffset = 0;
                                UInt32 WAVEARC2IDOffset = 0;
                                UInt32 WAVEARC3IDOffset = 0;
                                UInt32 WAVEARC4IDOffset = 0;
                                UInt32 SSEQOffset = 0;
                                UInt32 BANKOffset = 0;
                                UInt32 WAVEARC1Offset = 0;
                                UInt32 WAVEARC2Offset = 0;
                                UInt32 WAVEARC3Offset = 0;
                                UInt32 WAVEARC4Offset = 0;
                                UInt32 SSEQSize = 0;
                                UInt32 BANKSize = 0;
                                UInt32 WAVEARC1Size = 0;
                                UInt32 WAVEARC2Size = 0;
                                UInt32 WAVEARC3Size = 0;
                                UInt32 WAVEARC4Size = 0;


                                br.BaseStream.Position = SDATOffset[j] + 0x18;
                                InfoOffset = BitConverter.ToUInt32(br.ReadBytes(4), 0) + SDATOffset[j];

                                //Reads Offset of FAT in SDAT
                                br.BaseStream.Position = SDATOffset[j] + 0x20;
                                SDATFatOffset = BitConverter.ToUInt32(br.ReadBytes(4), 0) + SDATOffset[j];

                                //Reads Offset of SSEQ Rec
                                br.BaseStream.Position = InfoOffset + 0x8;
                                InfoSSEQRecOffset = BitConverter.ToUInt32(br.ReadBytes(4), 0) + InfoOffset;

                                //Reads Offset of BANK Rec
                                br.BaseStream.Position = InfoOffset + 0x10;
                                InfoBANKRecOffset = BitConverter.ToUInt32(br.ReadBytes(4), 0) + InfoOffset;

                                //Reads Offset of WAVEARC Rec
                                InfoWAVEARCRecOffset = BitConverter.ToUInt32(br.ReadBytes(4), 0) + InfoOffset;

                                //Reads Offset of SSEQ Entry
                                br.BaseStream.Position = InfoSSEQRecOffset + 4 + (k * 4);
                                SSEQIDOffset = BitConverter.ToUInt32(br.ReadBytes(4), 0) + InfoOffset;

                                if (SSEQIDOffset == InfoOffset)
                                {
                                    bw.BaseStream.Write(BitConverter.GetBytes(0), 0, 4);
                                    bw.BaseStream.Write(BitConverter.GetBytes(0), 0, 4);

                                    bw.BaseStream.Write(BitConverter.GetBytes(0), 0, 4);
                                    bw.BaseStream.Write(BitConverter.GetBytes(0), 0, 4);

                                    bw.BaseStream.Write(BitConverter.GetBytes(0), 0, 4);
                                    bw.BaseStream.Write(BitConverter.GetBytes(0), 0, 4);

                                    bw.BaseStream.Write(BitConverter.GetBytes(0), 0, 4);
                                    bw.BaseStream.Write(BitConverter.GetBytes(0), 0, 4);

                                    bw.BaseStream.Write(BitConverter.GetBytes(0), 0, 4);
                                    bw.BaseStream.Write(BitConverter.GetBytes(0), 0, 4);

                                    bw.BaseStream.Write(BitConverter.GetBytes(0), 0, 4);
                                    bw.BaseStream.Write(BitConverter.GetBytes(0), 0, 4);

                                    continue;
                                }

                                //Reads SSEQ ID
                                br.BaseStream.Position = SSEQIDOffset;
                                SSEQID = BitConverter.ToUInt16(br.ReadBytes(2), 0);

                                //Reads used BANK
                                br.BaseStream.Position = SSEQIDOffset + 4;
                                BANK = BitConverter.ToUInt16(br.ReadBytes(2), 0);

                                //Reads Offset of BANK Entry
                                br.BaseStream.Position = InfoBANKRecOffset + 4 + (BANK * 4);
                                BANKIDOffset = BitConverter.ToUInt32(br.ReadBytes(4), 0) + InfoOffset;

                                //Reads SSEQ ID
                                br.BaseStream.Position = BANKIDOffset;
                                BANKID = BitConverter.ToUInt16(br.ReadBytes(2), 0);

                                //Reads first used WAVEARC
                                br.BaseStream.Position = BANKIDOffset + 4;
                                WAVEARC1 = BitConverter.ToUInt16(br.ReadBytes(2), 0);

                                //Reads second used WAVEARC
                                WAVEARC2 = BitConverter.ToUInt16(br.ReadBytes(2), 0);

                                //Reads third used WAVEARC
                                WAVEARC3 = BitConverter.ToUInt16(br.ReadBytes(2), 0);

                                //Reads fourth used WAVEARC
                                WAVEARC4 = BitConverter.ToUInt16(br.ReadBytes(2), 0);

                                if (WAVEARC1 != 0xFFFF)
                                {
                                    //Reads Offset of first WAVEARC Entry
                                    br.BaseStream.Position = InfoWAVEARCRecOffset + 4 + (WAVEARC1 * 4);
                                    WAVEARC1IDOffset = BitConverter.ToUInt32(br.ReadBytes(4), 0) + InfoOffset;

                                    //Reads first WAVEARC ID
                                    br.BaseStream.Position = WAVEARC1IDOffset;
                                    WAVEARC1ID = BitConverter.ToUInt16(br.ReadBytes(2), 0);
                                }

                                if (WAVEARC2 != 0xFFFF)
                                {
                                    //Reads Offset of second WAVEARC Entry
                                    br.BaseStream.Position = InfoWAVEARCRecOffset + 4 + (WAVEARC2 * 4);
                                    WAVEARC2IDOffset = BitConverter.ToUInt32(br.ReadBytes(4), 0) + InfoOffset;

                                    //Reads second WAVEARC ID
                                    br.BaseStream.Position = WAVEARC2IDOffset;
                                    WAVEARC2ID = BitConverter.ToUInt16(br.ReadBytes(2), 0);
                                }

                                if (WAVEARC3 != 0xFFFF)
                                {
                                    //Reads Offset of third WAVEARC Entry
                                    br.BaseStream.Position = InfoWAVEARCRecOffset + 4 + (WAVEARC3 * 4);
                                    WAVEARC3IDOffset = BitConverter.ToUInt32(br.ReadBytes(4), 0) + InfoOffset;

                                    //Reads third WAVEARC ID
                                    br.BaseStream.Position = WAVEARC3IDOffset;
                                    WAVEARC3ID = BitConverter.ToUInt16(br.ReadBytes(2), 0);
                                }

                                if (WAVEARC4 != 0xFFFF)
                                {
                                    //Reads Offset of fourth WAVEARC Entry
                                    br.BaseStream.Position = InfoWAVEARCRecOffset + 4 + (WAVEARC4 * 4);
                                    WAVEARC4IDOffset = BitConverter.ToUInt32(br.ReadBytes(4), 0) + InfoOffset;

                                    //Reads fourth WAVEARC ID
                                    br.BaseStream.Position = WAVEARC4IDOffset;
                                    WAVEARC4ID = BitConverter.ToUInt16(br.ReadBytes(2), 0);
                                }

                                //Reads Offset and Size of SSEQ File
                                br.BaseStream.Position = SDATFatOffset + 12 + (SSEQID * 16);
                                SSEQOffset = BitConverter.ToUInt32(br.ReadBytes(4), 0) + SDATOffset[j];
                                SSEQSize = BitConverter.ToUInt32(br.ReadBytes(4), 0);

                                //Reads Offset and Size of BANK File
                                br.BaseStream.Position = SDATFatOffset + 12 + (BANKID * 16);
                                BANKOffset = BitConverter.ToUInt32(br.ReadBytes(4), 0) + SDATOffset[j];
                                BANKSize = BitConverter.ToUInt32(br.ReadBytes(4), 0);

                                if (WAVEARC1 != 0xFFFF)
                                {
                                    //Reads Offset and Size of first WAVEARC File
                                    br.BaseStream.Position = SDATFatOffset + 12 + (WAVEARC1ID * 16);
                                    WAVEARC1Offset = BitConverter.ToUInt32(br.ReadBytes(4), 0) + SDATOffset[j];
                                    WAVEARC1Size = BitConverter.ToUInt32(br.ReadBytes(4), 0);
                                }

                                if (WAVEARC2 != 0xFFFF)
                                {
                                    //Reads Offset and Size of second WAVEARC File
                                    br.BaseStream.Position = SDATFatOffset + 12 + (WAVEARC2ID * 16);
                                    WAVEARC2Offset = BitConverter.ToUInt32(br.ReadBytes(4), 0) + SDATOffset[j];
                                    WAVEARC2Size = BitConverter.ToUInt32(br.ReadBytes(4), 0);
                                }

                                if (WAVEARC3 != 0xFFFF)
                                {
                                    //Reads Offset and Size of third WAVEARC File
                                    br.BaseStream.Position = SDATFatOffset + 12 + (WAVEARC3ID * 16);
                                    WAVEARC3Offset = BitConverter.ToUInt32(br.ReadBytes(4), 0) + SDATOffset[j];
                                    WAVEARC3Size = BitConverter.ToUInt32(br.ReadBytes(4), 0);
                                }

                                if (WAVEARC4 != 0xFFFF)
                                {
                                    //Reads Offset and Size of fourth WAVEARC File
                                    br.BaseStream.Position = SDATFatOffset + 12 + (WAVEARC4ID * 16);
                                    WAVEARC4Offset = BitConverter.ToUInt32(br.ReadBytes(4), 0) + SDATOffset[j];
                                    WAVEARC4Size = BitConverter.ToUInt32(br.ReadBytes(4), 0);
                                }

                                bw.BaseStream.Write(BitConverter.GetBytes(SSEQOffset), 0, 4);
                                bw.BaseStream.Write(BitConverter.GetBytes(SSEQSize), 0, 4);

                                bw.BaseStream.Write(BitConverter.GetBytes(BANKOffset), 0, 4);
                                bw.BaseStream.Write(BitConverter.GetBytes(BANKSize), 0, 4);

                                bw.BaseStream.Write(BitConverter.GetBytes(WAVEARC1Offset), 0, 4);
                                bw.BaseStream.Write(BitConverter.GetBytes(WAVEARC1Size), 0, 4);

                                bw.BaseStream.Write(BitConverter.GetBytes(WAVEARC2Offset), 0, 4);
                                bw.BaseStream.Write(BitConverter.GetBytes(WAVEARC2Size), 0, 4);

                                bw.BaseStream.Write(BitConverter.GetBytes(WAVEARC3Offset), 0, 4);
                                bw.BaseStream.Write(BitConverter.GetBytes(WAVEARC3Size), 0, 4);

                                bw.BaseStream.Write(BitConverter.GetBytes(WAVEARC4Offset), 0, 4);
                                bw.BaseStream.Write(BitConverter.GetBytes(WAVEARC4Size), 0, 4);
                            }
                        }
                        bw.Close();
                    }
                    br.Close();
                    progressBar1.Value++;
                    Update();
                }
                MessageBox.Show("All SPS files have been successfully generated in\n" + Path.GetPathRoot(filePaths[0]) + "data\\NDS Music Player\\", "Completed", MessageBoxButtons.OK, MessageBoxIcon.Information);
                Close();
            }
        }
    }
}
