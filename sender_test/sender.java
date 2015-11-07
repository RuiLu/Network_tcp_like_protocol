/*
 * CSEE 4119 
 * Programming Assignment 2: Simple TCP-like transport-layer protocol
 * Author: Rui Lu
 * UNI: rl2784
 * 
 */
import java.io.*;
import java.lang.reflect.WildcardType;
import java.net.*;
import java.text.DecimalFormat;


public class sender {

	public static int SRC_PORT = 0;
	public static int DEST_PORT = 0;
	public static String DEST_IP = "";
	private static int WINDOW_SIZE = 1;
	private static int TIMEOUT = 300;
	private static int MAXTRIES = 100;
	public static String URG_DATA_POINTER = "0000000000000000";
	
	private int TOTAL_BYTE = 0;
	private int SEGMENT_NUM = 0;
	private int RETRANSMIT_NUM = 0;
	private int FILE_SIZE = 0;
	
	private static String datafile_path = "";
	private static String logfile_path = "";
	
	DatagramPacket inPacket = null;
	DatagramPacket outPacket = null;
	DatagramSocket socket = null;
	
	FileInputStream fis = null;
	byte[] message = new byte[WINDOW_SIZE];
	byte[] buf = new byte[WINDOW_SIZE-24];
	byte[] header = new byte[24];
	
	static long seqNum = 0;
	static long ackNum = 1;
	
	static double estimatedRTT = 0;
	static double sampleRTT = 0;
	static long sendTime = 0;
	static long rcvTime = 0;
	int RTTFlag = 1;
	
	private byte headerLength = 20 /4;
	private int UGR = 0;
	private int ACK = 1;
	private int PSH = 0;
	private int RST = 0;
	private int SYN = 0;
	private int FIN = 0;
	private int flag = 0;
	private int CHECKSUM = 0;
	private int URG_POINT = 0;
	private int PACKET_SIZE = 0;
	
	private static double ALPHA = 0.125;
	private static double BETA = 0.25;
	private static double DevRTT = 0;
	private static double TIMEOUT_INTERVAL = 0;
	
	DecimalFormat dfFlag = new DecimalFormat("000000");
	Tool tool = new Tool();
	
	public sender(){
		byte[] inData = new byte[24];
		inPacket = new DatagramPacket(inData, inData.length);
		
		int len = 0;
		int tries = 0;
		boolean isBroken = false;
		
		File dataFile = new File(datafile_path);
		if(!dataFile.exists()){
			System.out.println("File doesn't exist...");
			System.exit(0);
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
		
		BufferedWriter bw = null;
		ByteArrayOutputStream baos = null;
		
		
		try {
			baos = new ByteArrayOutputStream();
			bw = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(logFile, true)));
			fis = new FileInputStream(dataFile);
			FILE_SIZE = fis.available();
			System.out.println(FILE_SIZE);
			
			socket = new DatagramSocket(SRC_PORT);
			socket.setSoTimeout(TIMEOUT);
			
			/*
			 * send window size to receiver
			 */
			outPacket = new DatagramPacket(header, header.length, InetAddress.getByName(DEST_IP), DEST_PORT);
			header = tool.tcpHeader(SRC_PORT, DEST_PORT, seqNum, ackNum, headerLength, (byte)flag, WINDOW_SIZE, CHECKSUM, URG_POINT, PACKET_SIZE);
			outPacket.setData(header);
			socket.send(outPacket);
			while(true){
				try {
					socket.receive(inPacket);
					if(getAckNum(inPacket) == 0 && getSeqNum(inPacket) == 0) {
						break;
					}
				} catch (Exception e) {
					// TODO: handle exception
					socket.send(outPacket);
				}
			}
			
			/*
			 * try to receive the window-size confirmation message from receiver
			 */
			
			
			outPacket = new DatagramPacket(message, message.length, InetAddress.getByName(DEST_IP), DEST_PORT);
			
			while((len = fis.read(buf)) != -1) {
				TOTAL_BYTE += len;
				SEGMENT_NUM++;
				
				baos.reset();
				baos.write(buf, 0, len);
				byte[] data = baos.toByteArray();
				PACKET_SIZE = data.length;
				
				System.out.println("Wait for sending...");
				/*
				 * tcp header - 24 bytes
				 * data buf - 1000 bytes
				 * 
				 */
				String tcpFlags = Integer.toString(UGR) + Integer.toString(ACK) + Integer.toString(PSH) + 
								  Integer.toString(RST) + Integer.toString(SYN) + Integer.toString(FIN);
				flag = Integer.parseInt(Integer.valueOf(tcpFlags,2).toString());
				
				header = tool.tcpHeader(SRC_PORT, DEST_PORT, seqNum, ackNum, headerLength, (byte)flag, WINDOW_SIZE, CHECKSUM, URG_POINT, PACKET_SIZE);
				message = tool.byteMerge(header, data);
				CHECKSUM = tool.generateChecksum(message);
				header = tool.tcpHeader(SRC_PORT, DEST_PORT, seqNum, ackNum, headerLength, (byte)flag, WINDOW_SIZE, CHECKSUM, URG_POINT, PACKET_SIZE);
				message = tool.byteMerge(header, data);
				
				System.out.println(header.length + " " + len + " " + message.length);
				outPacket.setData(message);
				socket.send(outPacket);
				
				sendTime = System.currentTimeMillis();
				
				record(bw, Long.toString(sendTime), InetAddress.getLocalHost().getHostAddress(), DEST_IP, Long.toString(seqNum), Long.toString(ackNum), dfFlag.format(Double.parseDouble(tcpFlags)), "");
				
				ackNum++;
				seqNum += (long)len;
				
				System.out.println("sent...");
				
				try {
					socket.receive(inPacket);
					
					rcvTime = System.currentTimeMillis();
					estimatedRTT = countEstimatedRTT(sendTime, rcvTime);
					
					record(bw, Long.toString(rcvTime), inPacket.getAddress().getHostAddress(), InetAddress.getLocalHost().getHostAddress(), 
							  Long.toString(getSeqNum(inPacket)), Long.toString(getAckNum(inPacket)), dfFlag.format(Double.parseDouble(tcpFlags)), Double.toString(estimatedRTT));
					/*
					 * Determined ACK number is valid or not
					 */
					while(!isValidAck(inPacket, seqNum, len) && seqNum != FILE_SIZE){
						tries = 1;
						isBroken = true;
						while(tries < MAXTRIES) {
							System.out.println("Wrong ack, retransimit...");
							SEGMENT_NUM++;
							RETRANSMIT_NUM++;
							socket.setSoTimeout(TIMEOUT);
							header = tool.tcpHeader(SRC_PORT, DEST_PORT, seqNum - (long)len, ackNum - 1, headerLength, (byte)flag, WINDOW_SIZE, CHECKSUM, URG_POINT, PACKET_SIZE);
							message = tool.byteMerge(header, data);
							outPacket.setData(message);
							socket.send(outPacket);
							record(bw, Long.toString(sendTime), InetAddress.getLocalHost().getHostAddress(), DEST_IP, Long.toString(seqNum - (long)len), Long.toString(ackNum - 1), dfFlag.format(Double.parseDouble(tcpFlags)), "");
							
							sendTime = System.currentTimeMillis();
							
							try {
								socket.receive(inPacket);
								
								rcvTime = System.currentTimeMillis();
								estimatedRTT = countEstimatedRTT(sendTime, rcvTime);
								
								record(bw, Long.toString(rcvTime), inPacket.getAddress().getHostAddress(), InetAddress.getLocalHost().getHostAddress(), 
										   Long.toString(getSeqNum(inPacket)), Long.toString(getAckNum(inPacket)), dfFlag.format(Double.parseDouble(tcpFlags)), Double.toString(estimatedRTT));
								if(isValidAck(inPacket, seqNum, len)){
									isBroken = false;
									break;
								}
							} catch (Exception e2) {
								tries++;
							} finally {
								tries ++;
							}
						}
						if(tries == MAXTRIES) {
							break;
						}
					}
				} catch (Exception e) {
					tries = 1;
					isBroken = true;
					while(tries < MAXTRIES) {
						System.out.println("No response, retransimit...");
						SEGMENT_NUM++;
						RETRANSMIT_NUM++;
						socket.setSoTimeout(TIMEOUT);
						header = tool.tcpHeader(SRC_PORT, DEST_PORT, seqNum - (long)len, ackNum - 1, headerLength, (byte)flag, WINDOW_SIZE, CHECKSUM, URG_POINT, PACKET_SIZE);
						message = tool.byteMerge(header, data);
						outPacket.setData(message);
						socket.send(outPacket);
						
						sendTime = System.currentTimeMillis();
						
						record(bw, Long.toString(sendTime), InetAddress.getLocalHost().getHostAddress(), DEST_IP, Long.toString(seqNum - (long)len), Long.toString(ackNum - 1), dfFlag.format(Double.parseDouble(tcpFlags)), "");
						
						try {
							socket.receive(inPacket);
							
							rcvTime = System.currentTimeMillis();
							estimatedRTT = countEstimatedRTT(sendTime, rcvTime);
							record(bw, Long.toString(rcvTime), inPacket.getAddress().getHostAddress(), InetAddress.getLocalHost().getHostAddress(), 
									   Long.toString(getSeqNum(inPacket)), Long.toString(getAckNum(inPacket)), dfFlag.format(Double.parseDouble(tcpFlags)), Double.toString(estimatedRTT));
							if(isValidAck(inPacket, seqNum, len)){
								isBroken = false;
								break;
							}
							tries++;
						} catch (Exception e2) {
							tries++;
						}
					}
					
				} finally {
					if(tries == MAXTRIES) {
						break;
					}
				}
			}
			
			if(isBroken == true) {
				System.out.println("Failed to send file...");
			}
			else {
				System.out.println("Send FIN to end the process...");
				
				closeProcess(bw);
				
				System.out.println("Delivery completed successfully");
				System.out.println("Total bytes sent = " + TOTAL_BYTE);
				System.out.println("Segments sent = " + SEGMENT_NUM);
				System.out.println("Segements retransmitted = " + RETRANSMIT_NUM);
			}
			
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			try{
				if(socket != null) socket.close();
				if(fis != null) fis.close();
				if(bw != null) bw.close();
				if(baos != null) baos.close();
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
	}
	
	public void closeProcess(BufferedWriter bw){
		
		FIN = 1;
		ACK = 0;
		String tcpFlags = Integer.toString(UGR) + Integer.toString(ACK) + Integer.toString(PSH) + 
				  Integer.toString(RST) + Integer.toString(SYN) + Integer.toString(FIN);
		flag = Integer.parseInt(Integer.valueOf(tcpFlags,2).toString());
		
		try {
			header = tool.tcpHeader(SRC_PORT, DEST_PORT, 0, 0, (byte)5, (byte)flag, WINDOW_SIZE, 0, 0, 24);
			outPacket.setData(header);
			socket.send(outPacket);
			sendTime = System.currentTimeMillis();
			record(bw, Long.toString(sendTime), InetAddress.getLocalHost().getHostAddress(), DEST_IP, "0", "0", dfFlag.format(Double.parseDouble(tcpFlags)), "");
			
			socket.receive(inPacket);
			rcvTime = System.currentTimeMillis();
			estimatedRTT = countEstimatedRTT(sendTime, rcvTime);
			tcpFlags = dfFlag.format(Double.parseDouble(Integer.toBinaryString((inPacket.getData()[13])&63)));
			record(bw, Long.toString(rcvTime), inPacket.getAddress().getHostAddress(), InetAddress.getLocalHost().getHostAddress(), 
					   Long.toString(getSeqNum(inPacket)), Long.toString(getAckNum(inPacket)), dfFlag.format(Double.parseDouble(tcpFlags)), Double.toString(estimatedRTT));
			
			long rcvAck = getAckNum(inPacket);
			byte f1 = (byte)(inPacket.getData()[13]&63);
			if(rcvAck == 1 && f1 == 16) {
				socket.receive(inPacket);
				tcpFlags = dfFlag.format(Double.parseDouble(Integer.toBinaryString((inPacket.getData()[13])&63)));
				record(bw, Long.toString(rcvTime), inPacket.getAddress().getHostAddress(), InetAddress.getLocalHost().getHostAddress(), 
						   Long.toString(getSeqNum(inPacket)), Long.toString(getAckNum(inPacket)), dfFlag.format(Double.parseDouble(tcpFlags)), "");
				long rcvSeq = getSeqNum(inPacket);
				byte f2 = (byte)(inPacket.getData()[13]&63);
				if(f2 == 1) {
					FIN = 0;
					ACK = 1;
					tcpFlags = Integer.toString(UGR) + Integer.toString(ACK) + Integer.toString(PSH) + 
							  Integer.toString(RST) + Integer.toString(SYN) + Integer.toString(FIN);
					flag = Integer.parseInt(Integer.valueOf(tcpFlags,2).toString());
					header = tool.tcpHeader(SRC_PORT, DEST_PORT, 0, rcvSeq+1, (byte)5, (byte)flag, WINDOW_SIZE, 0, 0, 24);
					outPacket.setData(header);
					socket.send(outPacket);
					record(bw, Long.toString(sendTime), InetAddress.getLocalHost().getHostAddress(), DEST_IP, "0", "0", dfFlag.format(Double.parseDouble(tcpFlags)), "");
					try {
						Thread.sleep(2000);
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					
				}
			}
			
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
	
	public boolean isValidAck(DatagramPacket inPacket, long seqNum, int len){
		
		byte[] header = inPacket.getData();
		/*
		 * To check the tcpFlags
		 */
		DecimalFormat dfShort = new DecimalFormat("00000000");
		String hex = Byte.toString(header[13]);
		int i = Integer.parseInt(hex, 16);
		String binary = dfShort.format(Double.parseDouble(Integer.toBinaryString(i)));
		/*
		 * If ack bit is "1"
		 */
		if(binary.substring(3, 4).equals("1")){
			String ackStr = dfShort.format(Double.parseDouble(Integer.toBinaryString((header[8])&255)));
			ackStr += dfShort.format(Double.parseDouble(Integer.toBinaryString((header[9])&255)));
			ackStr += dfShort.format(Double.parseDouble(Integer.toBinaryString((header[10])&255)));
			ackStr += dfShort.format(Double.parseDouble(Integer.toBinaryString(header[11]&255)));
			long ack = (long)Integer.parseInt(ackStr, 2);
			if(ack == seqNum){

				return true;
			}
			else {
				return false;	
			}
		}
		return false;
		
	}
	
	public void record(BufferedWriter bw, String timeStamp, String s_ip, String d_ip, String seqNum, String ackNum, String flags, String estimatedRTT) {
		try {
			bw.write(timeStamp + ", " + s_ip + ", " + d_ip + ", #" + seqNum + ", #" + ackNum + ", " + flags + ", " + estimatedRTT +"\n");
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	public double countEstimatedRTT(long start, long end){
		/*
		 * calculate the estimated RTT
		 */
		double res = 0;
		sampleRTT = end - start;
		res = (1 - ALPHA) * estimatedRTT + ALPHA * sampleRTT; 

		/*
		 * calculate the TimeoutInterval
		 */
		DevRTT = (1 - BETA) * DevRTT + BETA * Math.abs(sampleRTT - estimatedRTT);
		TIMEOUT_INTERVAL = estimatedRTT + 4 * DevRTT;
		
		return res;
	}
	
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		datafile_path = args[0];
		DEST_IP = args[1];
		DEST_PORT = Integer.parseInt(args[2]);
		SRC_PORT = Integer.parseInt(args[3]);
		logfile_path = args[4];
		WINDOW_SIZE = Integer.parseInt(args[5]);
		new sender();
	}
}

class Tool{
	
	public String binaryString2hexString(String bString)
	{
		if (bString == null || bString.equals("") || bString.length() % 8 != 0)
			return null;
		StringBuffer tmp = new StringBuffer();
		int iTmp = 0;
		for (int i = 0; i < bString.length(); i += 4)
		{
			iTmp = 0;
			for (int j = 0; j < 4; j++)
			{
				iTmp += Integer.parseInt(bString.substring(i + j, i + j + 1)) << (4 - j - 1);
			}
			tmp.append(Integer.toHexString(iTmp));
		}
		return tmp.toString();
	}
	
	public byte[] byteMerge(byte[] byte_1, byte[] byte_2) {
		byte[] byte_3 = new byte[byte_1.length+byte_2.length];
		System.arraycopy(byte_1, 0, byte_3, 0, byte_1.length);
		System.arraycopy(byte_2, 0, byte_3, byte_1.length, byte_2.length);
		return byte_3;
	}
	
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


