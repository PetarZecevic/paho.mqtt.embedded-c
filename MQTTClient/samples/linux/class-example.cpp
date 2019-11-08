#define MQTTCLIENT_QOS2 1

#include <memory.h>

#include "MQTTClient.h"

#define DEFAULT_STACK_SIZE -1

#include "linux.cpp"


class Client
{
public:

	Client() : ipstack_(), cli_(ipstack_), arrivedCount_(0){}
	
	int connect(const char* hostname, int port)
	{
		printf("Connecting to %s:%d\n", hostname, port);
		int rc = ipstack_.connect(hostname, port);
		if (rc != 0)
			printf("rc from TCP connect is %d\n", rc);
	 	
		printf("MQTT connecting\n");
		MQTTPacket_connectData data = MQTTPacket_connectData_initializer;       
		data.MQTTVersion = 3;
		data.clientID.cstring = (char*)"test-class-client";
		rc = cli_.connect(data);
		if (rc != 0)
			printf("rc from MQTT connect is %d\n", rc);
		printf("MQTT connected\n");
		return rc;
	}
	
	int subscribe(const char* topic, bool nested=false)
	{
		int rc = -1;
		if(cli_.isConnected())
		{
			if(nested)
			{
				rc = cli_.subscribe(topic, MQTT::QOS2, 
					FP<void, MQTT::MessageData&>(this, &Client::messageArrived));		
			}
			else
			{
				FP<void, MQTT::MessageData&> fp;
				fp.attach(this, &Client::messageArrived);
				rc = cli_.subscribe(topic, MQTT::QOS2, fp);
			}  
			if (rc != 0)
				printf("rc from MQTT subscribe is %d\n", rc);
		}
		return rc;
	}
	
	int publish(const char* topic, MQTT::Message message)
	{
		int rc = -1;
		if(cli_.isConnected())
		{
			rc = cli_.publish(topic, message);
			if (rc != 0)
				printf("Error %d from sending message\n", rc);
    		cli_.yield(100);
		}
		return rc;
	}
	
	int unsubscribe(const char* topic)
	{
		int rc = -1;
		if(cli_.isConnected())
		{
			rc = cli_.unsubscribe(topic);
    		if (rc != 0)
        		printf("rc from unsubscribe was %d\n", rc);
        }
        return rc;
    }
    
    int disconnect()
    {
    	int rc = -1;
    	if(cli_.isConnected())
    	{
    		rc = cli_.disconnect();
			if (rc != 0)
		    	printf("rc from disconnect was %d\n", rc);	
			rc = ipstack_.disconnect();	
    	}
    	return rc;
    }
	
	int getMessageCount() const { return arrivedCount_; }
	
private:
	void messageArrived(MQTT::MessageData& mdata)
	{
		MQTT::Message &message = mdata.message;
		printf("Message %d arrived: qos %d, retained %d, dup %d, packetid %d\n", 
			++arrivedCount_, message.qos, message.retained, message.dup, message.id);
		printf("Payload %.*s\n", (int)message.payloadlen, (char*)message.payload);
	}
	IPStack ipstack_;
	MQTT::Client<IPStack, Countdown> cli_;
	int arrivedCount_;
};


int main(int argc, char* argv[])
{   
    const char* hostname = "localhost";
    int port = 1883;
    const char* topic = "test/class/topic";
    
    Client client;
    client.connect(hostname, port);
  	
  	MQTT::Message message;
  	float version = 3.0;
    // QoS 0
    char buf[100];
    sprintf(buf, "Hello World!  QoS 0 message from app version %f", version);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf)+1;
      
    client.subscribe(topic, true);
    client.publish(topic, message);
    
    // QoS 1
	printf("Now QoS 1\n");
    sprintf(buf, "Hello World!  QoS 1 message from app version %f", version);
    message.qos = MQTT::QOS1;
    message.payloadlen = strlen(buf)+1;
    client.publish(topic, message);
    
    // QoS 2
    sprintf(buf, "Hello World!  QoS 2 message from app version %f", version);
    message.qos = MQTT::QOS2;
    message.payloadlen = strlen(buf)+1;
    client.publish(topic, message);
    
    printf("Finishing with %d messages received\n", client.getMessageCount());
	return 0;
}
