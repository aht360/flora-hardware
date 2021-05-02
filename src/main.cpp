#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ctime>


// WIFI CONFIGURATION
const char* ssid = "NET APT 801";
const char* password =  "418311397";

// BOARD PINS
int light_sensor = 32;
int humidity_sensor = 33;
int sprinkle = 27;

// CONTS
const int SPRINKLE_DURATION_IN_MILLI = 5000;
const long long int DELAY_BETWEEN_SPRINKLES_IN_SECS = 3600;

String POST_JAR_DATA_ENDPOINT = "https://backend-flora.herokuapp.com/enviar_dados_vaso";
String GET_SPRINKLE_DATA_ENDPOINT = "https://backend-flora.herokuapp.com/editar_dados_vaso?name=Vaso%20Cristalino";

struct ApplicationState {
  long long int delay_between_sprinkles_in_secs = DELAY_BETWEEN_SPRINKLES_IN_SECS;
  long long int timestamp = 0;
  long long int timestamp_next_sprinkle = time(0) + DELAY_BETWEEN_SPRINKLES_IN_SECS;
  bool is_configuration_set = false;
};

ApplicationState application_state;

void set_sprinkle(){
  if (time(0) > application_state.timestamp_next_sprinkle) {
    printf("------------------\n");
    printf("REGANDOOOOOOOOOOOOOO\n");
    printf("------------------\n");
    digitalWrite(sprinkle, HIGH);
    delay(SPRINKLE_DURATION_IN_MILLI);
    digitalWrite(sprinkle, LOW);

    application_state.timestamp_next_sprinkle = time(0) + application_state.delay_between_sprinkles_in_secs;
  }
}


struct JarData {
  long long int humidity = 0;
  long long int luminosity = 0;
};

JarData my_jar;

void post_jar_data(JarData jarData) {
  HTTPClient http;

  http.begin(POST_JAR_DATA_ENDPOINT);

  const char request_template[] = "{\"humidity\": %lld, \"luminosity\": %lld, \"name\": \"Vaso Cristalino\"}";

  char request_body[50];
  sprintf(request_body, request_template, jarData.humidity, jarData.luminosity);


  String json(request_body);
  Serial.println(json);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(json);
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);
  Serial.println(http.getString());

  http.end();
}

long long int MAX_HUMIDITY = 4095, MAX_LUMINOSITY = 4095, MIN_HUMIDITY = 1200;

long long int max(long long a, long long b){
  return (a > b) ? a : b;
}

long long int handle_humidity(long long int humity_value){
  long long int humity_level = MAX_HUMIDITY - max(humity_value - MIN_HUMIDITY, 0LL);
  return min(100LL, (humity_level * 100LL) / (MAX_HUMIDITY - MIN_HUMIDITY));
}


long long int handle_luminosity(long long int luminosity_value) {
  return (luminosity_value * 100LL) / MAX_LUMINOSITY;
}

void update_jar_watering_configuration(){

  HTTPClient http;

  http.begin(GET_SPRINKLE_DATA_ENDPOINT);

  // read the status code and body of the response
  int statusCode = http.GET();

  String payload = http.getString();
  Serial.println("PAYLOAD");
  Serial.println(payload);
  long long int new_timestamp, new_delay_between_sprinkles;
  
  sscanf(payload.c_str(),"{\"timestamp\":%lld,\"tempo_de_rega_em_segundos\":%lld}", &new_timestamp, &new_delay_between_sprinkles);
  Serial.println();
  if(statusCode == 200 && new_timestamp != application_state.timestamp){
    Serial.println("ENTREEEI");
    application_state.timestamp = new_timestamp;
    application_state.delay_between_sprinkles_in_secs = new_delay_between_sprinkles;
    if(application_state.is_configuration_set == true) {
      application_state.timestamp_next_sprinkle = time(0);
    } else {
      application_state.timestamp_next_sprinkle = time(0) + application_state.delay_between_sprinkles_in_secs;
      application_state.is_configuration_set = true;
    }
  }
}

void connect_wifi(){
  WiFi.begin(ssid, password);
  
  while(WiFi.status() != WL_CONNECTED){
    Serial.println("Connecting to WiFi..");
    delay(500);
  }
  Serial.println("Connected to the WiFi network");
}

void setup() {
  Serial.begin(115200);
  pinMode(sprinkle, OUTPUT);
  connect_wifi();
}

void loop() {

  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
    long long int light_value = analogRead(light_sensor);
    long long int humidity_value = analogRead(humidity_sensor);

    my_jar.humidity = handle_humidity(humidity_value);
    my_jar.luminosity = handle_luminosity(light_value);

    printf("------------------\n");
    printf("application_state\n");
    Serial.print("timestamp -> ");
    Serial.println(application_state.timestamp);
    Serial.print("time(0) -> ");
    Serial.println(time(0));
    printf("delay_between_sprinkles - > %lld\n", application_state.delay_between_sprinkles_in_secs);
    printf("configuration_is_set - > %d\n", application_state.is_configuration_set);
    printf("timestamp next sprinkle - > %lld\n", application_state.timestamp_next_sprinkle);
    printf("------------------\n");

    // if(my_jar.humidity < 80) {
    //   Serial.println("Regar");
    //   digitalWrite(sprinkle, HIGH);
    // } else {
    //   digitalWrite(sprinkle, LOW);
    // }

    post_jar_data(my_jar);
    update_jar_watering_configuration();
    set_sprinkle();
  }

  delay(500);
}