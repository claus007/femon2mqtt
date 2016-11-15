#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "string.h"
#include <mosquitto.h>
#include <libdvbapi/dvbfe.h>


#define CLIENTID    "SatDish_%02d_%02d"
#define QOS         1
#define TIMEOUT     10000L


#define FE_STATUS_PARAMS (DVBFE_INFO_LOCKSTATUS|DVBFE_INFO_SIGNAL_STRENGTH|DVBFE_INFO_BER|DVBFE_INFO_SNR|DVBFE_INFO_UNCORRECTED_BLOCKS)

int                sleep_time = 1000000;
bool               verbose    = false;
struct mosquitto * mosq       = NULL;

char *    topic="FS20/SateliteDish";
int       adapter=0;
int       frontend=0;

char * topicName(const char * name)
{
    char * result=new char[100];
    snprintf(result,100,"%s_%02d_%02d/%s",topic,adapter,frontend,name);
    return result;
}

void my_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
    /* Pring all log messages regardless of level. */
    printf("%s\n", str);
}


bool connectMqtt(char * host,int port)
{
    char * offline="disconnected";
    mosq = mosquitto_new(NULL, true, NULL);
    if(!mosq)
    {
        fprintf(stderr, "Error: Out of memory.\n");
        return false;
    }
    mosquitto_log_callback_set(mosq, my_log_callback);

    char * topic=topicName("Status");
    mosquitto_will_set(mosq,topic,strlen(offline),offline);


    delete topic;
    //mosquitto_connect_callback_set(mosq, my_connect_callback);
    //mosquitto_message_callback_set(mosq, my_message_callback);
    //mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);

    if(mosquitto_connect(mosq, host, port, 60))
    {
        fprintf(stderr, "Unable to connect.\n");
        return false;
    }

    mosquitto_loop_start((mosq);
    return true;
}

void send(char * topic,char * payload)
{
    int mid;
    int qos=0;
    bool retain=true;

    topic=topicName(topic);

    mosquitto_publish(	mosq,
       &mid,
       topic,
       strlen(payload),
       payload,
       qos,
       retain);

    delete topic;

}

bool monitorDevice(adapter,frontend, topic)
{
    struct dvbfe_handle *fe;
    struct dvbfe_info fe_info;
    char *fe_type = "UNKNOWN";

    fe = dvbfe_open(adapter, frontend, 1);
    if (fe == NULL)
    {
        perror("opening frontend failed");
        return false;
    }

    while (1)
    {
        if (dvbfe_get_info(fe, FE_STATUS_PARAMS, &fe_info, DVBFE_INFO_QUERYTYPE_IMMEDIATE, 0) != FE_STATUS_PARAMS)
        {
            fprintf(stderr, "Problem retrieving frontend information: %m\n");
            return false;
        }
        send("Status","Ok");
        send("Lock",(fe_info.lock)?"Yes":"No");

        char buffer[100];
        sprintf(buffer,"%3u%%",(fe_info.signal_strength * 100) / 0xffff);
        send("Signal",buffer);

        sprintf(buffer,"%3u%%",(fe_info.snr * 100) / 0xffff);
        send("SignalNoiseRatio",buffer);

        usleep(sleep_time);
    }
}


int main(int argc, char *argv[])
{
    char    * host="localhost";
    int       port=1883;

    int       opterr = 0;

    int c;

    mosquitto_lib_init();

    while ((c = getopt (argc, argv, "hH:p:t:a:f:v")) != -1)
        switch (c)
        {
        case 'h':
            printf("%s -h        this help message", argv[0]);
            printf("   -H        m-host to connect to (default localhost)");
            printf("   -p        m-port to connect to (default 1883)");
            printf("   -t        m-topic (default 'FS20/SateliteDish_<Adapter>_<Frontend>)");
            printf("   -a        adapter (default 0)");
            printf("   -f        frontend (default 0)");
            printf("   -v        verbose mode");
            return 0;
        case 'H':
            host = optarg;
            break;
        case 'p':
            port = strtoul(optarg, NULL, 0);
            break;
        case 't':
            topic = optarg;
            break;
        case 'a':
            adapter = strtoul(optarg, NULL, 0);
            break;
        case 'f':
            frontend = strtoul(optarg, NULL, 0);
            break;
        case 'v':
            verbose = true;
            break;

        case '?':
            if (optopt == 'c')
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr,
                         "Unknown option character `\\x%x'.\n",
                         optopt);
            return 1;
        default:
            abort ();
        }

    if ( connectMqtt(host,port) == false)
    {
        fprintf (stderr, "Cannot connect to host" );
        return 2;
    }

    if (  monitorDevice(adapter,frontend, topic) == false)
    {
        return 3;
    }


    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;

}