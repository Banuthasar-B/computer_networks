#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SIZE 10
#define MAX_BITS 6000

/*==========================================================
                    HASH TABLE
==========================================================*/

char t_url[SIZE][50];
char t_ip[SIZE][20];
char t_mac[SIZE][20];

int hash(char str[])
{
    int sum = 0;

    int i;
    for(i=0; str[i]!='\0'; i++)
        sum += str[i];

    return sum % SIZE;
}

void insert(char url[], char ip[], char mac[])
{
    int index = hash(url);

    while(strlen(t_url[index]) != 0)
        index = (index + 1) % SIZE;

    strcpy(t_url[index], url);
    strcpy(t_ip[index], ip);
    strcpy(t_mac[index], mac);
}

void Websites()
{
    insert("google.com",
           "142.250.193.14",
           "3C:5A:B4:1D:9F:02");

    insert("youtube.com",
           "142.250.72.14",
           "A4:5E:60:D3:2B:19");

    insert("facebook.com",
           "157.240.22.35",
           "F0:2F:74:6B:88:11");

    insert("amazon.com",
           "205.251.242.103",
           "B8:27:EB:9A:3C:44");

    insert("wikipedia.org",
           "208.80.154.224",
           "00:1A:2B:3C:4D:5E");
}

void displayWebsites()
{
    printf("=====================================================\n");
    printf("     SIMPLE 4-LAYER NETWORK SIMULATOR (in binary)\n");
    printf("=====================================================\n\n");

    printf("---------------------------------------------------\n");
    printf(" HASH TABLE (URL -> IP -> MAC)\n");
    printf("---------------------------------------------------\n");

    int i;
    for(i=0;i<SIZE;i++)
    {
        if(strlen(t_url[i])!=0)
        {
            printf(" Slot %-1d : %-15s | %-15s | %s\n",
                   i,
                   t_url[i],
                   t_ip[i],
                   t_mac[i]);
        }
    }

    printf("---------------------------------------------------\n\n");
}

int search(char url[])
{
    int index = hash(url);
    int start = index;

    while(strlen(t_url[index])!=0)
    {
        if(strcmp(t_url[index],url)==0)
            return index;

        index=(index+1)%SIZE;

        if(index==start)
            break;
    }

    return -1;
}


void randomIP(char ip[])
{
    sprintf(ip,
            "%d.%d.%d.%d",
            rand()%223+1,
            rand()%256,
            rand()%256,
            rand()%254+1);
}

void randomMAC(char mac[])
{
    sprintf(mac,
            "%02X:%02X:%02X:%02X:%02X:%02X",
            rand()%256,
            rand()%256,
            rand()%256,
            rand()%256,
            rand()%256,
            rand()%256);
}


void charToBinary(unsigned char ch,char bin[])
{
    int i;
    for(i=7;i>=0;i--)
        bin[7-i]=((ch>>i)&1)+'0';

    bin[8]='\0';
}

void numberToBinary16(int number,char bin[])
{
    int i;
    for(i=15;i>=0;i--)
        bin[15-i]=((number>>i)&1)+'0';

    bin[16]='\0';
}

void ipToBinary(char ip[],char result[])
{
    int a,b,c,d;

    sscanf(ip,"%d.%d.%d.%d",&a,&b,&c,&d);

    char temp[9];

    result[0]='\0';

    int arr[4]={a,b,c,d};

    int i;
    for(i=0;i<4;i++)
    {
        charToBinary(arr[i],temp);
        strcat(result,temp);
    }
}

void macToBinary(char mac[],char result[])
{
    unsigned int x[6];

    sscanf(mac,
           "%x:%x:%x:%x:%x:%x",
           &x[0],
           &x[1],
           &x[2],
           &x[3],
           &x[4],
           &x[5]);

    char temp[9];

    result[0]='\0';

    int i;
    for(i=0;i<6;i++)
    {
        charToBinary(x[i],temp);
        strcat(result,temp);
    }
}


int main()
{
    srand(time(NULL));

    Websites();

    displayWebsites();

    char srcURL[50];
    char dstURL[50];

    printf("Enter SOURCE URL (example: google.com): ");
    scanf("%s",srcURL);

    printf("Enter DESTINATION URL (example: youtube.com): ");
    scanf("%s",dstURL);

    int srcIdx=search(srcURL);
    int dstIdx=search(dstURL);

    if(srcIdx==-1)
    {
        char ip[20];
        char mac[20];

        randomIP(ip);
        randomMAC(mac);

        insert(srcURL,ip,mac);

        srcIdx=search(srcURL);
    }

    if(dstIdx==-1)
    {
        char ip[20];
        char mac[20];

        randomIP(ip);
        randomMAC(mac);

        insert(dstURL,ip,mac);

        dstIdx=search(dstURL);
    }

    char srcIPBin[40];
    char dstIPBin[40];
    char srcMACBin[60];
    char dstMACBin[60];

    ipToBinary(t_ip[srcIdx],srcIPBin);
    ipToBinary(t_ip[dstIdx],dstIPBin);

    macToBinary(t_mac[srcIdx],srcMACBin);
    macToBinary(t_mac[dstIdx],dstMACBin);

    printf("\n----------------- ADDRESS RESOLUTION -----------------\n");

    printf("SOURCE      : %s -> IP %s -> MAC %s\n",
           t_url[srcIdx],
           t_ip[srcIdx],
           t_mac[srcIdx]);

    printf("   IP  binary  (32 bits): %s\n",srcIPBin);

    printf("   MAC binary  (48 bits): %s\n",srcMACBin);

    printf("DESTINATION : %s -> IP %s -> MAC %s\n",
           t_url[dstIdx],
           t_ip[dstIdx],
           t_mac[dstIdx]);

    printf("   IP  binary  (32 bits): %s\n",dstIPBin);

    printf("   MAC binary  (48 bits): %s\n",dstMACBin);

    printf("-------------------------------------------------------\n\n");

    char fileName[100];

    printf("Enter the file name that contains the message (example: message.txt): ");

    scanf("%s",fileName);

    FILE *fp=fopen(fileName,"r");

    if(fp==NULL)
    {
        printf("File not found!\n");
        return 0;
    }


char bits[MAX_BITS] = "";
char oneByte[9];
char ch;
char message[500] = "";

int msgIndex = 0;

printf("\n=============== APPLICATION LAYER ===============\n");

while((ch = fgetc(fp)) != EOF)
{
    if(ch == '\n' || ch == '\r')
        continue;

    message[msgIndex++] = ch;

    charToBinary((unsigned char)ch, oneByte);

    printf("  '%c'  ->  %s\n", ch, oneByte);

    strcat(bits, oneByte);
}

message[msgIndex] = '\0';

fclose(fp);

int totalBits = strlen(bits);

printf("\nStream      : %s\n", bits);
printf("Total Bits  : %d bits\n", totalBits);
printf("===================================================\n");



int sport = 1024 + rand() % (65535 - 1024 + 1);
int dport = 1024 + rand() % (65535 - 1024 + 1);

char srcPortBin[17];
char dstPortBin[17];

numberToBinary16(sport, srcPortBin);
numberToBinary16(dport, dstPortBin);

char transportStream[MAX_BITS];

transportStream[0] = '\0';

strcat(transportStream, srcPortBin);
strcat(transportStream, dstPortBin);
strcat(transportStream, bits);

int transportBits = 32 + totalBits;

printf("\n=============== TRANSPORT LAYER ===============\n");

printf("Source Port      : %d  (binary: %s)\n",
       sport,
       srcPortBin);

printf("Destination Port : %d  (binary: %s)\n",
       dport,
       dstPortBin);

printf("Stream           : %s\n", transportStream);

printf("Total Bits       : %d bits\n",
       transportBits);

printf("=================================================\n");



char networkStream[MAX_BITS];

networkStream[0] = '\0';

strcat(networkStream, srcIPBin);
strcat(networkStream, dstIPBin);
strcat(networkStream, bits);

int networkBits = 64 + totalBits;

printf("\n=============== NETWORK LAYER ===============\n");

printf("Source IP        : %s\n", srcIPBin);

printf("Destination IP   : %s\n", dstIPBin);

printf("Stream           : %s\n", networkStream);

printf("Total Bits       : %d bits\n\n", networkBits);



int packetSize = 16;

int packets = totalBits / packetSize;

if(totalBits % packetSize != 0)
    packets++;

char packetData[200][17];

int index = 0;

int i,j;
for(i = 0; i < packets; i++)
{
    for(j = 0; j < packetSize; j++)
    {
        if(index < totalBits)
            packetData[i][j] = bits[index++];
        else
            packetData[i][j] = '0';
    }

    packetData[i][16] = '\0';
}

printf("Total packets: %d\n");
printf("===============================================\n");


/*==========================================================
                DATA LINK LAYER
==========================================================*/

char trailer[] = "00000000";

char dataLinkStream[MAX_BITS];

dataLinkStream[0] = '\0';

strcat(dataLinkStream, srcMACBin);
strcat(dataLinkStream, dstMACBin);
strcat(dataLinkStream, srcIPBin);
strcat(dataLinkStream, dstIPBin);
strcat(dataLinkStream, srcPortBin);
strcat(dataLinkStream, dstPortBin);
strcat(dataLinkStream, bits);
strcat(dataLinkStream, trailer);

int dataLinkBits =
48 +
48 +
32 +
32 +
16 +
16 +
totalBits +
8;

printf("\n=============== DATA LINK LAYER ===============\n");

printf("Source MAC       : %s\n", srcMACBin);

printf("Destination MAC  : %s\n", dstMACBin);

printf("Stream           : %s\n", dataLinkStream);

printf("Total Bits       : %d bits\n\n", dataLinkBits);

printf("Data             : %s\n", bits);

printf("Src Port         : %s\n", srcPortBin);

printf("Dest Port        : %s\n", dstPortBin);

printf("Src IP           : %s\n", srcIPBin);

printf("Dest IP          : %s\n", dstIPBin);

printf("Src MAC          : %s\n", srcMACBin);

printf("Dest MAC         : %s\n", dstMACBin);

printf("Trailer          : %s\n", trailer);

printf("Full Stream      : %s%s%s%s%s%s%s%s\n",
       bits,
       srcPortBin,
       dstPortBin,
       srcIPBin,
       dstIPBin,
       srcMACBin,
       dstMACBin,
       trailer);

printf("\n");


int frameSize = 8;
int framesPerPacket = 2;
int totalFrames = packets * framesPerPacket;

printf("Total frames: %d\n\n", totalFrames);

printf("====Each frame and its content======\n\n");

int p,f;
for(p = 0; p < packets; p++)
{
    for(f = 0; f < framesPerPacket; f++)
    {
        char frameData[9];

        int start = f * frameSize;

        int i;
        for(i = 0; i < frameSize; i++)
        {
            frameData[i] = packetData[p][start + i];
        }

        frameData[8] = '\0';

        printf("---------------------------------------------\n");

        printf("Packet No : %d\n", p + 1);

        printf("Frame No  : %d\n", f + 1);

        printf("Source Port      : %s\n", srcPortBin);

        printf("Destination Port : %s\n\n", dstPortBin);

        printf("Source IP        : %s\n", srcIPBin);

        printf("Destination IP   : %s\n", dstIPBin);

        printf("Source MAC       : %s\n", srcMACBin);

        printf("Destination MAC  : %s\n", dstMACBin);

        printf("Frame Data       : %s\n", frameData);

        printf("Trailer          : %s\n", trailer);

        printf("Frame Stream     : ");

        printf("%s", trailer);

        printf("%s", srcMACBin);

        printf("%s", dstMACBin);

        printf("%s", frameData);

        printf("\n");

        printf("---------------------------------------------\n");
    }
}


printf("\n=============== SUMMARY ===============\n");

printf("Message              : \"%s\"\n", message);

printf("Application bits     : %d\n", totalBits);

printf("Transport layer bits : %d\n", transportBits);

printf("Network layer bits   : %d\n", networkBits);

printf("Data Link layer bits : %d\n", dataLinkBits);

printf("Total packets        : %d\n", packets);

printf("Total frames         : %d\n", totalFrames);

printf("=========================================\n");

return 0;
}
