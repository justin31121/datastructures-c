import static js.Io.*;
import js.Req;
import java.io.*;

class Main {
    public static void main(String[] args) throws IOException {
	long t = System.currentTimeMillis();
	final Req.Result result = Req.get("https://www.httpbin.org");
	if(!result.ok) {
	    exit(1);
	}
	t = System.currentTimeMillis() - t;
	println("Time taken", t, "ms");
    }
}
