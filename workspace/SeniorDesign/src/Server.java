import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.util.Iterator;
import java.util.Map;

import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpHandler;
import com.sun.net.httpserver.HttpServer;
import com.sun.net.httpserver.HttpContext;

public class Server
{

    public static void main(String[] args) throws Exception {
        HttpServer server = HttpServer.create(new InetSocketAddress(8000), 0);
        HttpContext context = server.createContext("/test", new MyHandler());
        context.getFilters().add(new ParameterFilter());
        server.setExecutor(null); // creates a default executor
        server.start();
        System.out.println("Started server.");
    }

    static class MyHandler implements HttpHandler {
        @Override
        public void handle(HttpExchange t) throws IOException {
        	System.out.println("Handling Request.");
            Map params = (Map)t.getAttribute("parameters");
            Iterator iterator = params.keySet().iterator();
            String response = "";
            while (iterator.hasNext())
            {
                String key = iterator.next().toString();
                String value = params.get(key).toString();
                response += "Key: " + key + ", Value: " + value + "\n";
                System.out.println(key + " " + value);
            }

            //String response = "This is the response";
            t.sendResponseHeaders(200, response.length());
            OutputStream os = t.getResponseBody();
            os.write(response.getBytes());
            os.close();
        }
    }

}