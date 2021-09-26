package java.lang;

public class String {
	private final char[] chars;
	
	public String() {
		this.chars = new char[0];
	}
	
	public String(char[] str) {
		this.chars = new char[str.length];
		for (int i = 0; i < str.length; i++) this.chars[i] = str[i];
	}

    public int length() {
        return this.chars.length;
    }

    public byte[] getBytes() {
		byte[] bytes = new byte[this.chars.length];
		for (int i = 0; i < this.chars.length; i++) bytes[i] = (byte) this.chars[i];
		return bytes;
	}
	
	public char[] toCharArray() {
		char[] newArray = new char[this.chars.length];
		for (int i = 0; i < newArray.length; i++) newArray[i] = this.chars[i];
		return newArray;
	}
}