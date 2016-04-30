#include "html.h"
#include "htmlCss.h"

// this code is bugged.
// void urldecode2(char *dst, const char *src)
// {
//   char a, b,c;
//   if (dst==NULL) return;
//   while (*src) {
//     if ((*src == '%') &&
//       ((a = src[1]) && (b = src[2])) &&
//       (isxdigit(a) && isxdigit(b))) {
//       if (a >= 'a')
//         a -= 'a'-'A';
//       if (a >= 'A')
//         a -= ('A' - 10);
//       else
//         a -= '0';
//       if (b >= 'a')
//         b -= 'a'-'A';
//       if (b >= 'A')
//         b -= ('A' - 10);
//       else
//         b -= '0';
//       *dst++ = 16*a+b;
//       src+=3;
//     } 
//     else {
//         c = *src++;
//         if(c=='+')c=' ';
//       *dst++ = c;
//     }
//   }
//   *dst++ = '\0';
// }

// String decodeB64(String text)
// {
//   char buff[100];
//   urldecode2(buff, text.c_str());
//   return String(buff);
// }

String encodeB64(String content)
{
  content.replace(" ","+");
  content.replace("!", "%21");
  content.replace("#", "%23");
  content.replace("$", "%24");
  content.replace("&", "%26");
  content.replace("'", "%27");
  content.replace("(", "%28");
  content.replace(")", "%29");
  content.replace("*", "%2A");
  content.replace("+", "%2B");
  content.replace(",", "%2C");
  content.replace("/", "%2F");
  content.replace(":", "%3A");
  content.replace(";", "%3B");
  content.replace("=", "%3D");
  content.replace("?", "%3F");
  content.replace("@", "%40");
  content.replace("[", "%5B");
  content.replace("]", "%5D");
  return content;

}

String decodeB64(String param)
{
  param.replace("+"," ");
  param.replace("%21","!");
  param.replace("%23","#");
  param.replace("%24","$");
  param.replace("%26","&");
  param.replace("%27","'");
  param.replace("%28","(");
  param.replace("%29",")");
  param.replace("%2A","*");
  param.replace("%2B","+");
  param.replace("%2C",",");
  param.replace("%2F","/");
  param.replace("%3A",":");
  param.replace("%3B",";");
  param.replace("%3D","=");
  param.replace("%3F","?");
  param.replace("%40","@");
  param.replace("%5B","[");
  param.replace("%5D","]");
  return param;
}

String getAvailableNetworks()
{
  String networks = "";
  int n = WiFi.scanNetworks();
  if(n == 0)
  {
    return String("None available <br>");
  }
  else
  {
    networks += "Number of networks: " + String(n) + "<br>";
    for(int i = 0; i < n; i++)
    {
      networks += "<li>";
      networks += (i+1) + ": ";
      networks += WiFi.SSID(i);
      networks += " (";
      networks += WiFi.RSSI(i);
      networks += ")";
      networks += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? "<br>" : "*<br>";
      networks += "</li>";
      delay(10);
      // limit search to 5 ssid's
      if(i == 5)
      {
        return networks;
      }
    }
  }
  return networks;
}

String generateWifiHtml()
{
  // get form data.
  if(server.args())
  {
    sta_ssid = server.arg("ssid");
    sta_pass = server.arg("pwd");
    board_name = server.arg("boardname");
    accessPin = server.arg("code").toInt();
    Serial.printf("sta_ssid: %s\n"
                  "sta_pass: %s\n"
                  "board_name: %s\n"
                  "accessPin: %d\n",
                  sta_ssid.c_str(),
                  sta_pass.c_str(),
                  board_name.c_str(),
                  accessPin);
    settingsStore();
  }

  String html = 
  "<!doctype html>"
  "<html lang=\"\">"
  "    <head>"
  "        <meta charset=\"utf-8\">"
  "        <meta http-equiv=\"x-ua-compatible\" content=\"ie=edge\">"
  "        <title>EspLight : WiFi</title>"
  "        <meta name=\"description\" content=\"\">"
  "        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
  "        <link rel=\"stylesheet\" href=\"/css\">"
  "    </head>"
  "    <body>"
  "    <div class=\"container\">"
  "      <div class=\"col-md-4\"></div>"
  "      <div class=\"col-md-4\">"
  "        <div class=\"row\">"
  "          <h1>EspLight</h1>"
  "        </div>"
  "        <div class=\"row\">"
  "          <div class=\"col-md-6\"><a href=\"wifisettings\" class=\"menu active\">WiFi</a></div>"
  "          <div class=\"col-md-6\"><a href=\"ledsettings\" class=\"menu\">LED</a></div>"
  "        </div>"
  "        <div class=\"row\">&nbsp;</div>"       
  "        <div class=\"row\">"
  "          <div class=\"panel\">"
  "            <h2>Settings</h2>"
  "            <div class=\"center\">"
  "              <form action=\"wifisettings\" method=\"POST\">"
  "              <label for=\"ssid\">SSID</label><br/><input type=\"text\" value=\"" + sta_ssid + "\" name=\"ssid\">"
  "              <label for=\"ssid\">password</label><br/><input type=\"text\" value=\"" + sta_pass + "\" name=\"pwd\">"
  "              <label for=\"ssid\">board name</label><br/><input type=\"text\" value=\"" + board_name + "\" name=\"boardname\">"
  "              <label for=\"ssid\">esplight code</label><br/><input type=\"text\" name=\"code\">"
  "              <input type=\"submit\" value=\"save\">"
  "              </form>"
  "            </div>"
  "          </div>"
  "          <div class=\"panel\">"
  "            <h2>Networks</h2>"
  "            <div class=\"center\">"
  "              <ul>" +
                   available_aps +
  "              </ul>"
  "            </div>"
  "          </div>"
  "        </div>"
  "      </div>"
  "      <div class=\"col-md-4\"></div>"
  "    </div>"
  "    </body>"
  "</html>"
  ;
  return html;
}

String generateLedHtml()
{

  if(server.args())
  {
    for(int i = 0; i < server.args(); i++)
    {
      Serial.print(server.arg(i));
      Serial.print(": ");
      Serial.println(server.arg(i).length());
    }
    if(server.arg("stripselect") == String("ws2812"))
    {
      stripselect = WS2812;
    }
    else if(server.arg("stripselect") == String("ws2801"))
    {
      stripselect = WS2801;
    }
    else if(server.arg("stripselect") == String("analog"))
    {
      stripselect = ANALOGSTRIP;
    }
    striplen = decodeB64(server.arg("striplen")).toInt();
    setupStrips(striplen);
    settingsStore();
  }

  String checked = "checked";
  String ws2801checked = (stripselect == WS2801) ? checked : "";
  String ws2812checked = (stripselect == WS2812) ? checked : "";
  String analogchecked = (stripselect == ANALOGSTRIP) ? checked : "";

  String html = 
  "<!doctype html>"
  "<html lang=\"\">"
  "    <head>"
  "        <meta charset=\"utf-8\">"
  "        <meta http-equiv=\"x-ua-compatible\" content=\"ie=edge\">"
  "        <title>EspLight : WiFi</title>"
  "        <meta name=\"description\" content=\"\">"
  "        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
  "        <link rel=\"stylesheet\" href=\"/css\">"
  "    </head>"
  "    <body>"
  "    <div class=\"container\">"
  "      <div class=\"col-md-4\"></div>"
  "      <div class=\"col-md-4\">"
  "        <div class=\"row\">"
  "          <h1>EspLight</h1>"
  "        </div>"
  "        <div class=\"row\">"
  "          <div class=\"col-md-6\"><a href=\"wifisettings\" class=\"menu\">WiFi</a></div>"
  "          <div class=\"col-md-6\"><a href=\"ledsettings\" class=\"menu active\">LED</a></div>"
  "        </div>"
  "        <div class=\"row\">&nbsp;</div> "
  "        <div class=\"row\">"
  "          <form action=\"\" method=\"POST\">"
  "          <div class=\"panel\">"
  "            <h2>Strip type</h2>"
  "            <div class=\"center\">"
  "              <input type=\"radio\" name=\"stripselect\" id=\"analog\" value=\"analog\"" + analogchecked + "><label for=\"analog\">Analog</label><br/>"
  "              <input type=\"radio\" name=\"stripselect\" id=\"ws2812\" value=\"ws2812\"" + ws2812checked + "><label for=\"ws2812\">WS2812</label><br/>"
  "              <input type=\"radio\" name=\"stripselect\" id=\"ws2801\" value=\"ws2801\"" + ws2801checked + "><label for=\"ws2801\">WS2801</label><br/>"
  "            </div>"
  "          </div>"
  "          <div class=\"panel\">"
  "            <h2>Leds</h2>"
  "            <div class=\"center\">"
  "              <label for=\"lednumber\">Number of leds</label><input type=\"text\" value=\"" + String(striplen) + "\" name=\"striplen\"><br/> "
  "              <input type=\"submit\" value=\"save\" />"
  "            </div>"
  "          </div> "
  "          </form>"
  "        </div>"
  "      </div>"
  "      <div class=\"col-md-4\"></div>"
  "    </div>"
  "    </body>"
  "</html>"
  ;
  return html;
}

void handleCss()
{
  server.send_P(200, "text/css", tidy_css);
}

void handleWiFiSettings()
{
  server.send(200, "text/html", generateWifiHtml());
}

void handleLedSettings()
{
  server.send(200, "text/html", generateLedHtml());
}

void handleRoot()
{
  handleWiFiSettings();
}