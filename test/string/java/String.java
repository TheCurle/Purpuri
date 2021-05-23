package java;

public class String {
	private final char[] chars;
	
	public String() {
		chars = new char[0];
	}
	
	public String(char[] chars) {
		this.chars = new char[chars.length];
		java.lang.System.arraycopy(chars, 0, this.chars, 0, chars.length);
	}
	
	public byte[] getBytes() {
		byte[] bytes = new byte[chars.length];
		for (int i = 0; i < chars.length; i++) bytes[i] = (byte) chars[i];
		return bytes;
	}
	
	public char[] toCharArray() {
		char[] chars = new char[this.chars.length];
		java.lang.System.arraycopy(this.chars, 0, chars, 0, chars.length);
		return chars;
	}
}
