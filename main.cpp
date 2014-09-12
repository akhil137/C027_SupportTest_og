#include "mbed.h"
#include "GPS.h"
#include "MDM.h"

#define SIMPIN "1234"
#define APN "phone"
//! Set the user name for your APN, or NULL if not needed
#define USERNAME NULL
//! Set the password for your APN, or NULL if not needed
#define PASSWORD NULL 

int main() 
{
    
    //instantiante modem object
    MDMSerial mdm; 

    //initialize some 'arrays?'
    MDMParser::DevStatus devStatus = {};
    MDMParser::NetStatus netStatus = {};

    //initialize the modem device
    bool mdmOk = mdm.init(SIMPIN, &devStatus);
    mdm.dumpDevStatus(&devStatus);

//TCP transfers thru cellular modem
#if 0   
    
    if (mdmOk) 
    {

        //register modem to network
        mdmOk = mdm.registerNet(&netStatus);
        mdm.dumNetStatus(&netStatus);
        // join apn and get ip
        MDMParser::IP ip = mdm.join(APN,USERNAME,PASSWORD);
        // if we have an IP then create a TCP connection
        if (ip != NOIP)
        {
            mdm.dumpIP(ip); 
            // create a TCP socket
            int socket=mdm.socketSocket(MDMParser::IPPROTO_TCP);
            // send strings (char array) thru socket
            // neg value means socket not open
            if (socket >=0 ) 
            {

                //what? is this a timeout?
                mdm.socketSetBlocking(socket, 10000);
                //generic TCP socket usage after instantianting socket
                //1. connnect socket to server/port
                //2. if connection succesful
                //3. send string thru socket
                //4. recieve string thru socket
                //5. close socket
                //6. free socket

                //generic UDP socket usage after instantianting socket
                //1. send string thru socket to server ip/port
                //2. recieve string
                //3. free socket


                // now connect socket to server on port 80
                if (mdm.socketConnect(socket,"mbed.org",80)) 
                {
                    const char http[]="GET";
                    //send string thru socket
                    mdm.socketSend(socket,http,sizeof(http)-1);
                    //recieve the reply in a bufer
                    char buf[512] = "";
                    ret=mdm.socketRecv(socket,buf,sizeof(buf)-1);
                    //if successful then print reply
                    if (ret > 0)
                        printf("\"%*s\"\n",ret,buf);
                    //close socket
                    mdm.socketClose(socket);
                }
                //free socket
                mdm.socketFree(socket);
            }

        }
    }
#endif

//GPS localization
    GPSI2C gps;
    char msg[512] = ""; //a buffer for gps and later for sms
    char link[128] = "";
    bool abort = false;
    int ret;
    double la = 0, lo = 0;
    char ch;
    const int wait = 100;
    int loopCount = 0;

    //get some gps data and parse it until we get a lat/long
    while (!abort)
    {
        loopCount+=1;
        //while((ret = gps.getMessage(msg,sizeof(msg)))>0)
        if ((ret = gps.getMessage(msg,sizeof(msg)))>0)
        {
            printf("This is the return value of gps.getMessage: %i\n",ret);
            int len = LENGTH(ret);
            if ((PROTOCOL(ret) == GPSParser::NMEA) && (len > 6))
            {
                if (!strncmp("$GPGLL", msg, 6)) 
                {
                    if (gps.getNmeaAngle(1,msg,len,la) && 
                        gps.getNmeaAngle(3,msg,len,lo) && 
                        gps.getNmeaItem(6,msg,len,ch) && ch == 'A')
                    {
                        printf("GPS Location: %.5f %.5f\r\n", la, lo); 
                        sprintf(link, "I am here!\n"
                                  "https://maps.google.com/?q=%.5f,%.5f", la, lo); 
                        abort=true;
                        printf("it took this many loops: %i\n",loopCount);
                    }
                }
            }
            //wait_ms(wait);
           // loopCount += 1;
            //some abort condition like la,lo don't change much
            //if (loopCount > 10)
              //  abort = true;
        }
            
    }
    gps.powerOff();
    loopCount=0;
    abort=false;




//SMS thru cellular modem
    
    
    //initialize a string buffer
    
    printf("deleting all messages: %d\n",mdm.smsDelete());
//#if 0
    while (!abort)
    {
        if (mdmOk)
        {
            //check network status
            if (mdm.checkNetStatus(&netStatus))
            {
                mdm.dumpNetStatus(&netStatus, fprintf, stdout);
            
            }

            //read sms 
            //1. get unread sms and store in a temp array to cycle thru
            int ix[1]; //sms store
            int n=mdm.smsList("REC UNREAD",ix,1); //number of unread?
            printf("number of unread messages:%i\n",n);
            char num[32]; //hold number of sender
            //2. now parse the sms store items into a num and msg buffer
            mdm.smsRead(ix[0], num, msg, sizeof(msg));
            //3. can delete the sms store item if needed
            mdm.smsDelete(ix[0]);
            //4. do some logic with the read msg
            const char* reply = "ublox sez: unrecognized cmd";
            if (strstr(msg,"loc"))
            {
                reply= *link ? link : "i don't know";
                mdm.smsSend(num,reply);
                strcpy(msg,"");  //clear the string
            }
            else if (strstr(msg, "end"))
            {
                abort = true, reply = "bye bye";
                mdm.smsSend(num,reply);
            }
            else if (loopCount==0) //send 'unrecognized cmd' but only once
            {
                mdm.smsSend(num,reply);
                loopCount+=1;
            }
            
        }
                
        wait_ms(wait);
            
    }
    mdm.powerOff(); 
//#endif
    return 0;
}
