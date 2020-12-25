#include "DHT.h" //librairie pour le capteur de température et d'humidité
#define DHTPIN 2 // broche ou l'on a branche le capteur
#define DHTTYPE DHT22 // DHT 22 (AM2302)

DHT dht(DHTPIN, DHTTYPE);//déclaration du capteur
 
#include <Servo.h> //librairie pour le servo-moteur
#define pinServo 4
Servo monServo;

#include <ESP8266WiFi.h>

#ifndef STASSID
#define STASSID "[ID]"    //Remplacer par l'id de votre connexion
#define STAPSK  "[MDP]"   //Remplacer par le mot de passe de votre connexion
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

const int photoR = 0;  //Initialisation de la photorésistance et de la led
const int led = 5;
int val;

int autom = false;  //Variable pour le mode automatique 

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);

void setup() {
  Serial.begin(115200);

  monServo.attach(pinServo);  //Initialisation du servo-moteur
  monServo.write(0); //Rotation à 0°

  pinMode(led,OUTPUT); //Led initialisation

  Serial.println("DHTxx test!");
  dht.begin();

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print(F("Connecting to "));
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();
  Serial.println(F("WiFi connected"));

  // Start the server
  server.begin();
  Serial.println(F("Server started"));

  // Print the IP address
  Serial.println(WiFi.localIP());
}



void loop() {

  
  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  Serial.println(F("new client"));

  client.setTimeout(1000); // default is 1000

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(F("request: "));
  Serial.println(req);

  // Match the request
  client.print(F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n"));

  if (req.indexOf(F("/ON")) != -1) {  //Si la connexion à la page "/ON" ou "/OFF", changement de la variable autom
     autom = true;
  }
  else if (req.indexOf(F("/OFF")) != -1) {
     autom = false;
  }
  
  if (req.indexOf(F("/0")) != -1) {   //Si la connexion à la page "/0", affichage des données d'humidité et de température
    client.print(F(" Voici les donnees : "));
    
    float h = dht.readHumidity();//on lit l'hygrometrie
    float t = dht.readTemperature();//on lit la temperature en celsius (par defaut)
    if (isnan(h) || isnan(t))
     {
       client.println("Impossible de lire les donnees!");
       return;
     }
     else{
         client.print("Humidite: ");
         client.print(h);
         client.print(" %\t");
         client.print("Temperature: ");
         client.print(t);
         client.print(" *C <br>");
          
     }
     client.print("Revenir a la page principale : <a href=\"home"); //lien pour le retour à la page principale
     
     client.print("\">retour</a> !<br>");  
     
  } 
  else if (req.indexOf(F("/Arroser")) != -1) { //Si la connexion à la page "/Arroser", arrosage de la plante grâce au servo moteur
     client.println("Arrosage de la plante...<br>");
     monServo.write(150);
     delay(2000); //attente de 2sec
     monServo.write(0);
     client.println("Plante arrosee !!!<br>");
     client.print("Revenir a la page principale : <a href=\"home");
     client.print("\">retour</a> !<br>"); 
     
  }
  else if (req.indexOf(F("/")) != -1) { //Connexion à la page principale, 
     // Send the response to the client
  // it is OK for multiple small client.print/write,
  // because nagle algorithm will group them into one single packet
  client.print("Mode auto : <a href=\"/ON\">ON</a> <a href=\"/OFF\">OFF</a> !<br>");
  client.print("Etat actuel du mode auto : ");
  if(autom == true)
  {
    client.print("On");
  }
  else{
    client.print("Off");
  }
  client.print("!<br>");
  client.print("Recevoir les donnees Humidite et Temperature : <a href=\"/0\">Acceder</a> !<br>");
  client.print("Arroser la plante : <a href=\"/Arroser\">Arroser</a> !<br>");
  }
  
  else {
    Serial.println(F("invalid request"));
  }
  
  // read/ignore the rest of the request
  // do not client.flush(): it is for output only, see below
  while (client.available()) {
    // byte by byte is not very efficient
    client.read();
  }

  if(autom == true){
    if (dht.readHumidity()<50) //Vérificatrion que l'humidité est pas inférieur à 50%, Arrosage si c'est le cas
    {
      client.println("<br>Arrosage de la plante grace au mode auto...<br>");
       monServo.write(150);
       delay(2000);
       monServo.write(0);
       client.println("Plante arrosee !!!<br>");
    }
  }
  
  val = analogRead(photoR); //Vérification de la luminosité de la lampe et allumer la Led si la photorésistance n'est pas éclairé
  if (val<500)
  {
    client.println("<br>La plante n est pas assez eclaire : ");
    client.println(val/10);
    client.print(" %");
    digitalWrite(led,HIGH); 
  }
  else{
    client.println("<br>La plante est bien eclaire : ");
    client.println(val/10);
    client.print(" %");
    digitalWrite(led,LOW);
  }

  
  

   client.stop(); //Déconnexion du client après avoir reçu la requète
   Serial.println("Client Disconnected.");
  // The client will actually be *flushed* then disconnected
  // when the function returns and 'client' object is destroyed (out-of-scope)
  // flush = ensure written data are received by the other side
}
