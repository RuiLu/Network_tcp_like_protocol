/*
 * CSEE 4119 
 * Programming Assignment 2: Simple TCP-like transport-layer protocol
 * Author: Rui Lu
 * UNI: rl2784
 * 
 */
import java.io.*;
import java.net.*;
import java.text.DecimalFormat;
import java.util.Arrays;

public class receiver {
	
	public static int SRC_PORT = 0;
	public static int DEST_PORT = 0;
	public static String DEST_IP = "";
	private static int WINDOW_SIZE = -1;
	
	private BufferedWriter bw = null;
	private FileOutputStream fos = null;
	private DatagramPacket inPacket = null;
	private DatagramPacket outPacket = null;
	private DatagramPacket outPacket_forSize = null;
	private DatagramSocket socket = null;
	private byte[] outData = null;
	private byte[] inData = null;
	private byte[] outData_forSize = null;
	
	private static String datafile_path = "";
	private static String logfile_path = "";
	
	Tool tool = new Tool();
	DecimalFormat dfFlag = new DecimalFormat("000000");
	
	private int CHECKSUM = 0;
	private int URG_POINT = 0;
	private int FLAG = 16;
	
	private long seqNum = 0;
	private long ackNum = 0;
	private long ackFlag = 0;
	
	private int test_1 = 0;
	private int test_2 = 0;
	private int UGR = 0;
	private int ACK = 1;
	private int PSH = 0;
	private int RST = 0;
	private int SYN = 0;
	private int FIN = 0;
	private int flag = 0;
	
	public receiver(){
		
		File dataFile = new File(datafile_path);
		if(!dataFile.exists()){
			try {
				dataFile.createNewFile();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		
		File logFile = new File(logfile_path);
		if(!logFile.exists()){
			try {
				logFile.createNewFile();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		
		
		try {
			
			socket = new DatagramSocket(SRC_PORT);
			
			fos = new FileOutputStream(dataFile);
			bw = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(logFile, true)));
			
			long size = 0;
			
			/*
			 * receive window size from sender
			 */
			inData = new byte[24];
			inPacket = new DatagramPacket(inData, inData.length);
			socket.receive(inPacket);
			WINDOW_SIZE = getWindowSize(inPacket);
			inData = new byte[WINDOW_SIZE];
			inPacket = new DatagramPacket(inData, inData.length);
			
			outData_forSize = tool.tcpHeader(0, 0, 0, 0, (byte)0, (byte)0, 0, 0, URG_POINT, 24);
			outPacket_forSize = outPacket = new DatagramPacket(outData_forSize, outData_forSize.length, InetAddress.getByName(DEST_IP), DEST_PORT);
			socket.send(outPacket_forSize);
			
			while(true){
				socket.receive(inPacket);
				
				String tcpFlags = dfFlag.format(Double.parseDouble(Integer.toBinaryString((inPacket.getData()[13])&63)));
				record(bw, Long.toString(System.currentTimeMillis()), inPacket.getAddress().getHostAddress(), InetAddress.getLocalHost().getHostAddress(), 
						   Long.toString(getSeqNum(inPacket)), Long.toString(getAckNum(inPacket)), dfFlag.format(Double.parseDouble(tcpFlags)));
				
				/*
				 * Receive repeat window size packet
				 */
				if(inPacket.getData().length == 24 && getAckNum(inPacket) != 0) {
					System.out.println("you shouldn't appear");
					socket.send(outPacket_forSize);
					continue;
				}
				
//				record(bw, Long.toString(System.currentTimeMillis()), inPacket.getAddress().getHostAddress(), InetAddress.getLocalHost().getHostAddress(), 
//						   Long.toString(getSeqNum(inPacket)), Long.toString(getAckNum(inPacket)));
				
				if((inPacket.getData()[13]&63) == 1) {
					break;
				}
				
				size = getPacketSize(inPacket);
				
				test_1 = getCheckSum(inPacket);
				byte[] temp = Arrays.copyOfRange(inPacket.getData(), 0, inPacket.getLength());
				test_2 = tool.generateChecksum(temp);
				/*
				 * If checksum is the same, indicating the data in this packet is intact.
				 * Otherwise, we should wait for next incoming intact packet
				 */
				if(test_1 == test_2){
					/*
					 * Avoid duplicate packets writing into the file again
					 */
					if(getAckNum(inPacket) > ackFlag) {
						outData = responseData(fos, inPacket, size);
						ackFlag++;
					}
					outPacket = new DatagramPacket(outData, outData.length, InetAddress.getByName(DEST_IP), DEST_PORT);
					socket.send(outPacket);
					record(bw, Long.toString(System.currentTimeMillis()), InetAddress.getLocalHost().getHostAddress(), DEST_IP, Long.toString(seqNum), Long.toString(ackNum), dfFlag.format(Double.parseDouble(tcpFlags)));
				}
				else {
					continue;
				}
			}
			
			closeProcess();
			
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			try {
				if(socket != null) socket.close();
				if(fos != null) fos.close();
				if(bw != null) bw.close();
			} catch (Exception e2) {
				// TODO: handle exception
				e2.printStackTrace();
			}
			System.out.println("Delivery completed successfully");
		}
	}
	
	public void closeProcess(){
		byte[] header = new byte[24];
		try {
			FIN = 0;
			ACK = 1;
			String tcpFlags = Integer.toString(UGR) + Integer.toString(ACK) + Integer.toString(PSH) + 
					  Integer.toString(RST) + Integer.toString(SYN) + Integer.toString(FIN);
			flag = Integer.parseInt(Integer.valueOf(tcpFlags,2).toString());
			
			header = tool.tcpHeader(SRC_PORT, DEST_PORT, 0, 1, (byte)5, (byte)flag, WINDOW_SIZE, CHECKSUM, URG_POINT, 24);
			outPacket.setData(header);
			socket.send(outPacket);
			record(bw, Long.toString(System.currentTimeMillis()), InetAddress.getLocalHost().getHostAddress(), DEST_IP, Long.toString(getSeqNum(outPacket)), Long.toString(getAckNum(outPacket)), dfFlag.format(Double.parseDouble(tcpFlags)));
			
			FIN = 1;
			ACK = 0;
			tcpFlags = Integer.toString(UGR) + Integer.toString(ACK) + Integer.toString(PSH) + 
					  Integer.toString(RST) + Integer.toString(SYN) + Integer.toString(FIN);
			flag = Integer.parseInt(Integer.valueOf(tcpFlags,2).toString());
			
			header = tool.tcpHeader(SRC_PORT, DEST_PORT, 2, 0, (byte)5, (byte)flag, WINDOW_SIZE, CHECKSUM, URG_POINT, 24);
			outPacket.setData(header);
			socket.send(outPacket);
			record(bw, Long.toString(System.currentTimeMillis()), InetAddress.getLocalHost().getHostAddress(), DEST_IP, Long.toString(getSeqNum(outPacket)), Long.toString(getAckNum(outPacket)), dfFlag.format(Double.parseDouble(tcpFlags)));
			
			socket.receive(inPacket);
			tcpFlags = dfFlag.format(Double.parseDouble(Integer.toBinaryString((inPacket.getData()[13])&63)));
			record(bw, Long.toString(System.currentTimeMillis()), inPacket.getAddress().getHostAddress(), InetAddress.getLocalHost().getHostAddress(), 
					   Long.toString(getSeqNum(inPacket)), Long.toString(getAckNum(inPacket)), dfFlag.format(Double.parseDouble(tcpFlags)));
			byte f1 = (byte)(inPacket.getData()[13]&63);
			if(f1 == 16){
				System.out.println("Receiver is over...");
			}
			
		} catch (Exception e) {
			e.printStackTrace();
		}
		
	}
	
	public void writeData(FileOutputStream fos, DatagramPacket inPacket, long size){
		
		byte[] inData = inPacket.getData();
		byte[] data = Arrays.copyOfRange(inData, 24, 24 + (int)size);
		
		try {
			fos.write(data, 0, data.length);
			fos.flush();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	
	}
	
	public byte[] responseData(FileOutputStream fos, DatagramPacket inPacket, long size){
		DecimalFormat dfShort = new DecimalFormat("00000000");
		byte[] inData = inPacket.getData();
		byte[] header = Arrays.copyOfRange(inData, 0, 24);
		
		/*
		 * Seq Num
		 */
		String seqStr = dfShort.format(Double.parseDouble(Integer.toBinaryString((header[4])&255)));
		seqStr += dfShort.format(Double.parseDouble(Integer.toBinaryString((header[5])&255)));
		seqStr += dfShort.format(Double.parseDouble(Integer.toBinaryString((header[6])&255)));
		seqStr += dfShort.format(Double.parseDouble(Integer.toBinaryString(header[7]&255)));
		
		int seq = Integer.parseInt(seqStr, 2);
		
		ackNum = (long)(seq + inData.length - header.length);
		/*
		 * Write data to file
		 */
		writeData(fos, inPacket, size);
		
		/*
		 * Ack Num
		 */
		String ackStr = dfShort.format(Double.parseDouble(Integer.toBinaryString((header[8])&255)));
		ackStr += dfShort.format(Double.parseDouble(Integer.toBinaryString((header[9])&255)));
		ackStr += dfShort.format(Double.parseDouble(Integer.toBinaryString((header[10])&255)));
		ackStr += dfShort.format(Double.parseDouble(Integer.toBinaryString(header[11]&255)));
		int ack = Integer.parseInt(ackStr, 2);
		
		seqNum = (long)ack;
		
		header = tool.tcpHeader(SRC_PORT, DEST_PORT, seqNum, ackNum, (byte)5, (byte)FLAG, WINDOW_SIZE, CHECKSUM, URG_POINT, 24);
		return header;
	}
	
	public void record(BufferedWriter bw, String timeStamp, String s_ip, String d_ip, String seqNum, String ackNum, String flags) {
		try {
			bw.write(timeStamp + ", " + s_ip + ", " + d_ip + ", #" + seqNum + ", #" + ackNum + ", " + flags + "\n");
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	public long getSeqNum(DatagramPacket inPacket){
		byte[] header = inPacket.getData();
		DecimalFormat dfShort = new DecimalFormat("00000000");
		String seqStr = dfShort.format(Double.parseDouble(Integer.toBinaryString((header[4])&255)));
		seqStr += dfShort.format(Double.parseDouble(Integer.toBinaryString((header[5])&255)));
		seqStr += dfShort.format(Double.parseDouble(Integer.toBinaryString((header[6])&255)));
		seqStr += dfShort.format(Double.parseDouble(Integer.toBinaryString(header[7]&255)));
		int seq = Integer.parseInt(seqStr, 2);
		return (long)seq;
	}
	
	public long getAckNum(DatagramPacket inPacket){
		byte[] header = inPacket.getData();
		DecimalFormat dfShort = new DecimalFormat("00000000");
		String ackStr = dfShort.format(Double.parseDouble(Integer.toBinaryString((header[8])&255)));
		ackStr += dfShort.format(Double.parseDouble(Integer.toBinaryString((header[9])&255)));
		ackStr += dfShort.format(Double.parseDouble(Integer.toBinaryString((header[10])&255)));
		ackStr += dfShort.format(Double.parseDouble(Integer.toBinaryString(header[11]&255)));
		int ack = Integer.parseInt(ackStr, 2);
		return (long)ack;
	}
	
	public long getPacketSize(DatagramPacket inPacket){
		byte[] header = inPacket.getData();
		DecimalFormat dfShort = new DecimalFormat("00000000");
		String sizeStr = dfShort.format(Double.parseDouble(Integer.toBinaryString((header[20])&255)));
		sizeStr += dfShort.format(Double.parseDouble(Integer.toBinaryString((header[21])&255)));
		sizeStr += dfShort.format(Double.parseDouble(Integer.toBinaryString((header[22])&255)));
		sizeStr += dfShort.format(Double.parseDouble(Integer.toBinaryString(header[23]&255)));
		int size = Integer.parseInt(sizeStr, 2);
		return (long)size;
	}
	
	public int getCheckSum(DatagramPacket inPacket){
		byte[] header = inPacket.getData();
		DecimalFormat dfShort = new DecimalFormat("00000000");
		String checksumStr = dfShort.format(Double.parseDouble(Integer.toBinaryString((header[16])&255)));
		checksumStr += dfShort.format(Double.parseDouble(Integer.toBinaryString((header[17])&255)));
		int checkSum = Integer.parseInt(checksumStr, 2);
		return checkSum;
	}
	
	public int getWindowSize(DatagramPacket inPacket){
		byte[] header = inPacket.getData();
		DecimalFormat dfShort = new DecimalFormat("00000000");
		String sizeStr = dfShort.format(Double.parseDouble(Integer.toBinaryString((header[14])&255)));
		sizeStr += dfShort.format(Double.parseDouble(Integer.toBinaryString((header[15])&255)));
		int size = Integer.parseInt(sizeStr, 2);
		return size;
	}
	
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		datafile_path = args[0];
		SRC_PORT = Integer.parseInt(args[1]);
		DEST_IP = args[2];
		DEST_PORT = Integer.parseInt(args[3]);
		logfile_path = args[4];
		
		new receiver();
	}

}

class Tool{
	public byte[] tcpHeader(int srcPort, int dstPort, long seqNum, long ackNum,
			byte data_offset, byte flags, int window, int checksum, int urg_pointer, int packet_size)
	{
		byte[] header = new byte[24];
		
		header[0] = (byte)((srcPort>>8)&255);
		header[1] = (byte)(srcPort&255);
		header[2] = (byte)((dstPort>>8)&255);
		header[3] = (byte)(dstPort&255);
		header[4] = (byte)((seqNum>>24)&255);
		header[5] = (byte)((seqNum>>16)&255);
		header[6] = (byte)((seqNum>>8)&255);
		header[7] = (byte)(seqNum&255);
		header[8] = (byte)((ackNum>>24)&255);
		header[9] = (byte)((ackNum>>16)&255);
		header[10] = (byte)((ackNum>>8)&255);
		header[11] = (byte)(ackNum&255);
		header[12] = (byte)((data_offset&15)<<4);
		header[13] = (byte)(flags&63);
		header[14] = (byte)((window>>8)&255);
		header[15] = (byte)(window&255);
		header[16] = (byte)((checksum>>8)&255);
		header[17] = (byte)(checksum&255);
		header[18] = (byte)((urg_pointer>>8)&255);
		header[19] = (byte)(urg_pointer&255);
		header[20] = (byte)(packet_size>>24&255);
		header[21] = (byte)(packet_size>>16&255);
		header[22] = (byte)(packet_size>>8&255);
		header[23] = (byte)(packet_size&255);
		return header;
	}
	
	public int generateChecksum(byte[] byteArray) {

		int sum = 0; 
		
		// if the byte array has odd number of octets, padding a zero byte
		byte[] stream ;
		if (byteArray.length % 2 != 0) {
			stream = new byte[byteArray.length+1];
			for (int i=0; i< byteArray.length; i++) {
				stream[i] = byteArray[i];
			}
			stream[byteArray.length] = 0;
		} else {
			stream = new byte[byteArray.length];
			for (int i=0; i< byteArray.length; i++)
				stream[i] = byteArray[i];
		}		
		
		stream[16] = 0;
		stream[17] = 0;
		// adjacent 8 bit words are stored as a short, 
		// sum up the 16 bit shorts and compute 1's complement for checksum
		for (int c=0; c < stream.length; c=c+2 ) {
			int firstByte = Byte.valueOf(stream[c]).intValue();
			
			// to convert it to unsigned value
			firstByte = firstByte&255;
			int shifted = (firstByte<<8);
			int nextbyte = stream[c+1]&255;
			int twoBytesGrouping = (shifted + (stream[c+1]&255));
			sum = sum + twoBytesGrouping;
		}
		
		//adding the carried over bits to the checksum to keep it a 16 bit word
		while (sum > 65535)
			sum = sum - 65536 + 1;
		
		//compute one's complement of sum
		sum = (~sum&0xFFFF);		
		
		return sum ;
	}
}