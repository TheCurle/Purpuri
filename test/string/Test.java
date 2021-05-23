import java.String;
import java.System;

public class Test {
	public static int num = 0;
	
	public static int EntryPoint() {
		Test1.main();
		main(new String[0]);
		return num;
	}
	
	public static void main(String[] args) {
		String s = new String(new char[]{'h', 'e', 'l', 'l', 'o'});
		String s1 = new String();
		for (byte aByte : s.getBytes()) {
			char[] chars = s1.toCharArray();
			char[] chars1 = new char[chars.length + 1];
			System.arraycopy(chars, 0, chars1, 0, chars.length);
			chars1[chars.length] = (char) aByte;
			s1 = new String(chars1);
		}
	}
}
