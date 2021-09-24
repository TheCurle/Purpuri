public class If {
    public static int EntryPoint() {
        int x = 5;

        if(x > 6) 
            return 0;
        else if(x > 5)
            return 1;
        
        if(x < 4) 
            return 2;
        else if(x == 5)
            return 3;
        
        return 4;
    }
}