package java;

public class System {
	public static void arraycopy(Object src,  int  srcPos,
								 Object dest, int destPos,
								 int length) {
		Object[] in = (Object[]) src;
		Object[] out = (Object[]) dest;
		for (int i = srcPos; i < srcPos + length; i++) out[i - srcPos + destPos] = in[i];
	}
}
