import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpHandler;
import com.sun.net.httpserver.HttpServer;
import com.sun.net.httpserver.HttpContext;

public class Server
{
    public static void main(String[] args) throws Exception
    {
        HttpServer server = HttpServer.create(new InetSocketAddress(8000), 0);
        HttpContext context = server.createContext("/", new MyHandler());
        context.getFilters().add(new ParameterFilter());
        server.setExecutor(null); // creates a default executor
        server.start();
        System.out.println("Started server.");
    }

    static class MyHandler implements HttpHandler
    {
        private final String path = "/home/zach/Desktop/www/file.txt";
        @Override
        public void handle(HttpExchange t) throws IOException
        {
        	
            if ("get".equalsIgnoreCase(t.getRequestMethod()))
            {
            	handleGet(t);

            }
            else if ("post".equalsIgnoreCase(t.getRequestMethod()))
            {
            	handlePost(t);
            }
        }
        
        private void handleGet(HttpExchange t) throws IOException
        {
        	System.out.println("Handling GET request.");
            File file = new File(path);
            //URI uri = t.getRequestURI();
            //File file = new File(root + uri.getPath()).getCanonicalFile();
            if (!file.isFile())
            {
              // Object does not exist or is not a file: reject with 404 error.
              String response = "404 (Not Found)\n";
              t.sendResponseHeaders(404, response.length());
              OutputStream os = t.getResponseBody();
              os.write(response.getBytes());
              os.close();
            } else {
              // Object exists and is a file: accept with response code 200.
              t.sendResponseHeaders(200, 0);
              OutputStream os = t.getResponseBody();
              FileInputStream fs = new FileInputStream(file);
              final byte[] buffer = new byte[0x10000];
              int count = 0;
              while ((count = fs.read(buffer)) >= 0) {
                os.write(buffer,0,count);
              }
              fs.close();
              os.close();
            }
        }
        
        
        private void handlePost(HttpExchange t) throws IOException
        {
        	System.out.println("Handling POST request.");
            Map params = (Map)t.getAttribute("parameters");
            Iterator iterator = params.keySet().iterator();
            String response = "";
            while (iterator.hasNext())
            {
                String key = iterator.next().toString();
                String value = params.get(key).toString();
                response += key + ": " + value + " | ";
                System.out.println(key + ": " + value);
            }
            if (!response.isEmpty())
            {
	            List<String> lines = Arrays.asList(response);
	            Path file = Paths.get(path);
	            Files.write(file, lines, Charset.forName("UTF-8"), StandardOpenOption.APPEND);
            }
            
            //String response = "This is the response";
            t.sendResponseHeaders(200, response.length());
            OutputStream os = t.getResponseBody();
            os.write(response.getBytes());
            os.close();
        }
    }

}