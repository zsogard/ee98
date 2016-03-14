import com.sun.net.httpserver.Filter;
import com.sun.net.httpserver.HttpExchange;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.URI;
import java.net.URLDecoder;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;


public class ParameterFilter extends Filter
{

    @Override
    public String description()
    {
        return "Parses the requested URI for parameters";
    }

    @Override
    public void doFilter(HttpExchange exchange, Chain chain)
        throws IOException
    {
        if ("post".equalsIgnoreCase(exchange.getRequestMethod()))
        {
        	parsePostParameters(exchange);
        }
        chain.doFilter(exchange);
    }

    private void parsePostParameters(HttpExchange exchange)
        throws IOException
    {		
    		//create key/value map, parse query string, add map to the httpexchange
            Map parameters = new LinkedHashMap();
            URI requestedUri = exchange.getRequestURI();
            String query = requestedUri.getRawQuery();
            parseQuery(query, parameters);
            exchange.setAttribute("parameters", parameters);
    }

     @SuppressWarnings("unchecked")
     private void parseQuery(String query, Map parameters)
         throws UnsupportedEncodingException
     {
    	 System.out.println(query);
         if (query != null) {
             String pairs[] = query.split("[&]");

             for (String pair : pairs) {
                 String param[] = pair.split("[=]");

                 String key = null;
                 String value = null;
                 if (param.length > 0) {
                     key = URLDecoder.decode(param[0],
                         System.getProperty("file.encoding"));
                 }

                 if (param.length > 1) {
                     value = URLDecoder.decode(param[1],
                         System.getProperty("file.encoding"));
                 }

                 if (parameters.containsKey(key)) {
                     Object obj = parameters.get(key);
                     if(obj instanceof List) {
                         List values = (List)obj;
                         values.add(value);
                     } else if(obj instanceof String) {
                         List values = new ArrayList();
                         values.add((String)obj);
                         values.add(value);
                         parameters.put(key, values);
                     }
                 } else {
                     parameters.put(key, value);
                 }
             }
         }
    }
}