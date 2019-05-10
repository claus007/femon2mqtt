/*
   Copyright 2019 Claus Ilginnis

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

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

int                sleep_time = 10*1000000;
bool               verbose    = false;
struct mosquitto * mosq       = NULL;

const char * topic="FS20/SateliteDish";
int       adapter=0;
int       frontend=0;

char * topicName(const char * name)
{
    char * result=new char[100];
    snprintf(result,100,"%s_%02d_%02d/%s",topic,adapter,frontend,name);
    return result;
}

void my_log_callback(struct mosquitto *, void *, int , const char *str)
{
    /* Pring all log messages regardless of level. */
    printf("%s\n", str);
}


bool connectMqtt(const char * host,int port)
{
    const char * offline="disconnected";
    mosq = mosquitto_new(NULL, true, NULL);
    if(!mosq)
    {
        fprintf(stderr, "Error: Out of memory.\n");
        return false;
    }
    // let mosquitto printf
    mosquitto_log_callback_set(mosq, my_log_callback);

    // if we get disconected status will go to disconnected
    char * topic=topicName("Status");
    mosquitto_will_set(mosq,topic,strlen(offline),offline,0,true);


    delete topic;
    //mosquitto_connect_callback_set(mosq, my_connect_callback);
    //mosquitto_message_callback_set(mosq, my_message_callback);
    //mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);

    if(mosquitto_connect(mosq, host, port, 60))
    {
        fprintf(stderr, "Unable to connect.\n");
        return false;
    }

    // this loop is neccessary to do the
    // keep alive messages
    mosquitto_loop_start(mosq);
    return true;
}

void send(const char * topic,const char * payload)
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

    if (verbose)
        printf("sending: %s --> %s\n",topic,payload);

    delete topic;

}

bool monitorDevice(int adapter,int frontend)
{
    struct dvbfe_handle *fe;
    struct dvbfe_info fe_info;
    //const char *fe_type = "UNKNOWN";

    fe = dvbfe_open(adapter, frontend, 1);
    if (fe == NULL)
    {
        perror("opening frontend failed");
        return false;
    }

    while (1)
    {
        // the real query
        if (dvbfe_get_info(fe, (dvbfe_info_mask)FE_STATUS_PARAMS, &fe_info, DVBFE_INFO_QUERYTYPE_IMMEDIATE, 0) != FE_STATUS_PARAMS)
        {
            fprintf(stderr, "Problem retrieving frontend information: %m\n");
            //return false;
        }
        
        // keep status ok - despite last will
        send("Status","Ok");
        send("Lock",(fe_info.lock)?"Yes":"No");

        char buffer[100];
        sprintf(buffer,"%3u%%",(fe_info.signal_strength * 100) / 0xffff);
        send("Signal",buffer);

        sprintf(buffer,"%3u%%",(fe_info.snr * 100) / 0xffff);
        send("SignalNoiseRatio",buffer);

        // time to sleep
        usleep(sleep_time);
    }
}


int main(int argc, char *argv[])
{
    const char * host="localhost";
    int          port=1883;

    int c;

    mosquitto_lib_init();

    while ((c = getopt (argc, argv, "hH:p:t:a:f:v")) != -1)
        switch (c)
        {
        case 'h':
            printf("%s -h        this help message\n", argv[0]);
            printf("   -H        m-host to connect to (default localhost)\n");
            printf("   -p        m-port to connect to (default 1883)\n");
            printf("   -t        m-topic (default 'FS20/SateliteDish_<Adapter>_<Frontend>)\n");
            printf("   -a        adapter (default 0)\n");
            printf("   -f        frontend (default 0)\n");
            printf("   -v        verbose mode\n");
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

    if (  monitorDevice(adapter,frontend) == false)
    {
        return 3;
    }


    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;

}
