import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
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
    	//initialize log
    	File f = new File("/home/zach/Desktop/www/log.txt");
    	f.delete();
    	f.createNewFile();
    	
    	//start server at port 8000, "/"
        HttpServer server = HttpServer.create(new InetSocketAddress(8000), 0);
        HttpContext context = server.createContext("/", new MyHandler());
        context.getFilters().add(new ParameterFilter());
        server.setExecutor(null); // creates a default executor
        server.start();
        System.out.println("Started server.");
    }
    

    static class MyHandler implements HttpHandler
    {
        private final String path = "/home/zach/Desktop/www/log.txt";
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
            }
            else
            {
              // Object exists and is a file
              //read log file and create HTML table out of it
              String response = "<!DOCTYPE HTML><html><head><title>Sensor Readings</title>";
              //CSS
              response += "<style>p {font-family: Georgia, Tahoma, Verdana, Serif; font-size: 18px; margin-left: 5%; margin-right: 5%; text-align: center;}";
              response += "h1 { text-align: center;}";
              response += "table.center { margin-left:auto; margin-right:auto;}</style>";
              
              response +="</head><body><h1>Sensor Readings</h1>";
              
              //get current timestamp
              DateFormat dateFormat = new SimpleDateFormat("MM/dd/yyyy HH:mm:ss");
         	  Date date = new Date();
         	  response += "<p>" + "Last retrieved at: " + dateFormat.format(date) + "</p>";
         	  
         	  response += "<table border=\"1\" class=\"center\">";
         	  response += "<tr><td>Brightness</td><td>Moisture</td><td>pH</td><td>Temperature</td></tr>";
              //read each line of the file
         	  BufferedReader br = new BufferedReader(new FileReader(path));
              try {
            	  String line = br.readLine();
                  while (line != null) {
                	  	//split each key value pair (comma-separated)
                    	String[] pairs = line.split(",");
                    	String cell = "";
                    	response += "<tr>";
                    	for (int i = 0; i < pairs.length; i++)
                    	{
                    		//split the key and value of this pair and get the value
                    		cell = pairs[i].split("=")[1];
                    		response += "<td>" + cell + "</td>";
                    	}
                     	response += "</tr>";
                     	//read the next line
                        line = br.readLine();
                  }
              } finally {
                  br.close();
              }
              response += "</table></body></html>";
              t.sendResponseHeaders(200, response.length());
              OutputStream os = t.getResponseBody();
              os.write(response.getBytes());
              os.close(); 
            }
        }
        
        
        private void handlePost(HttpExchange t) throws IOException
        {
        	System.out.println("Handling POST request.");
            String response = "";
     	    
     	    //iterate through map
            Map params = (Map)t.getAttribute("parameters");
            Iterator iterator = params.keySet().iterator();
            while (iterator.hasNext())
            {
                String key = iterator.next().toString();
                String value = params.get(key).toString();
                response += key + "=" + value + ",";
                System.out.println(key + ": " + value);
            }
            
            //append key/value pairs to log file
            if (!response.isEmpty())
            {
	            List<String> lines = Arrays.asList(response);
	            Path file = Paths.get(path);
	            Files.write(file, lines, Charset.forName("UTF-8"), StandardOpenOption.APPEND);
            }
            
            //echo the response back to the sender
            t.sendResponseHeaders(200, response.length());
            OutputStream os = t.getResponseBody();
            os.write(response.getBytes());
            os.close();
        }
    }

}