
#include "common.h"

#define CFG_TRX_SIZE		(128 * 1024)

  
static int detect_mii(int skfd, char *ifname)
{
   struct ifreq ifr;
   unsigned short *data, mii_val;
   //unsigned phy_id;

   /* Get the vitals from the interface. */
   strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

   if (ioctl(skfd, SIOCGMIIPHY, &ifr) < 0)
      {
         (void) close(skfd);
         return 2;
      }

   data = (unsigned short *)(&ifr.ifr_data);
   //phy_id = data[0];
   data[1] = 1;

   if (ioctl(skfd, SIOCGMIIREG, &ifr) < 0)
   {
       return -1;
   }

   mii_val = data[3];
   return(((mii_val & 0x0016) == 0x0004) ? 1 : 0);
}








int netSetTRXSize(int fd,int size)
{
	int on = 1;
	int rxsockbufsz;
	if(fd < 0)
	{
		return -1;
	}
	rxsockbufsz = (size <= 0)?CFG_TRX_SIZE:size;
	setsockopt(fd,SOL_SOCKET,SO_RCVBUF,(const char*)&rxsockbufsz, sizeof(rxsockbufsz));
	setsockopt(fd,SOL_SOCKET,SO_SNDBUF,(const char*)&rxsockbufsz, sizeof(rxsockbufsz));
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof (on));
	return 0;
}

int netSetTCPNoDelay(int fd,int nodelay)
{
	int on = nodelay;
	if(fd < 0)
	{
		return -1;
	}
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&on, sizeof(on));
	return 0;
}



int netRTPReceive(int ip, int svr_port,int clt_port)
{
	struct sockaddr_in sin;
	int rxsockbufsz;
	int s;
	int on = 1;
	s = socket (AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
	{
	        return -1;
	}
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof (on));
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons ((uint16_t)clt_port);
	if (bind (s, (struct sockaddr *) &sin, sizeof (sin)))
	{
	        closesocket(s);
	        return -1;
	}
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = ip;
	sin.sin_port = htons(svr_port);
	connect(s, (struct sockaddr *)&sin, sizeof(sin));
	rxsockbufsz = CFG_TRX_SIZE;
	setsockopt (s,SOL_SOCKET,SO_RCVBUF,(const char*)&rxsockbufsz, sizeof(rxsockbufsz));
	setsockopt (s,SOL_SOCKET,SO_SNDBUF,(const char*)&rxsockbufsz, sizeof(rxsockbufsz));
/*
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 500 * 1000;		
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));
*/
	return s;
}



int netRTPConnect(char *hostname, int port)
{
	struct sockaddr_in sin;
	int rxsockbufsz;
	int s;
	int on = 1;	
	s = socket (AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
	{
		return -1;
	}
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof (on));
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons ((uint16_t)port);
	if (bind (s, (struct sockaddr *) &sin, sizeof (sin)))
	{
		closesocket(s);
		return -1;
	}
	rxsockbufsz = CFG_TRX_SIZE;
	setsockopt (s,SOL_SOCKET,SO_RCVBUF,(const char*)&rxsockbufsz, sizeof(rxsockbufsz));
	setsockopt (s,SOL_SOCKET,SO_SNDBUF,(const char*)&rxsockbufsz, sizeof(rxsockbufsz));
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 500 * 1000;		
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));
	return s;
}



int netCreateUDPMulticastReceive(uint32_t src_ip,uint16_t src_port,uint32_t dst_ip,uint16_t dst_port)
{
	int s;
	struct sockaddr_in my;
	struct ip_mreq mreq;
	int ret;
	int on = 1;
	s = socket (AF_INET, SOCK_DGRAM, 0);
	if (s < 0) 
	{
		return -1;
	}
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof (on));
/*
	iRecvBuf = CFG_TRX_SIZE;
	setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&iRecvBuf, sizeof(iRecvBuf));
	setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*)&iRecvBuf, sizeof(iRecvBuf));
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 500 * 1000;		
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));
*/
	my.sin_family = AF_INET;
	my.sin_addr.s_addr = 0;
	my.sin_port = htons(dst_port);
	ret = bind(s, (struct sockaddr *)&my, sizeof(my));
	if (ret != 0) 
	{
		closesocket(s);
		return -1;
	}
/*
	if(src_port)
	{
		my.sin_family = AF_INET;
		my.sin_addr.s_addr = src_ip;
	    	my.sin_port = htons(src_port);
	    	ret = connect(s, (struct sockaddr *)&my, sizeof(my));
	    	if (ret != 0)
	    	{
	            closesocket(s);
	            return -1;
		}
	}
*/
	mreq.imr_multiaddr.s_addr = dst_ip;
	mreq.imr_interface.s_addr = src_ip;
	ret = setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreq, sizeof (mreq));
	if (ret != 0) 
	{
		closesocket(s);
		return -1;
	}
	return s;
}



int netRTCPConnect(int client_port, int server_port, uint8_t* server_hostname)
{
	struct sockaddr_in sin;
	uint32_t server_ip = netGetIPFrom((char*)server_hostname);
	int s;
	int on = 1;
	s = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s == -1)
	{
		return -1;
	}
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof (on));
	int iRecvBuf = CFG_TRX_SIZE;
	setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&iRecvBuf, sizeof(iRecvBuf));
	setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*)&iRecvBuf, sizeof(iRecvBuf));
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 500 * 1000;		
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons ((uint16_t)client_port);
	
	if (bind (s, (struct sockaddr *) &sin, sizeof (sin)))
	{
		closesocket(s);
		return -1;
	}
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = server_ip;
	sin.sin_port = htons ((uint16_t)server_port);
	if (connect (s, (struct sockaddr *) &sin, sizeof (sin)) < 0)
	{
		closesocket(s);
		return -1;
	}
	return s;
}




int netRTSPConnect(int srcip,uint32_t ip,uint32_t port)
{
	struct sockaddr_in to;
	struct sockaddr_in addr;
	int s;
	int ret;
	int bufsize;
	int on = 1;
	s = socket (AF_INET, SOCK_STREAM,0);
	if(s < 0)
	{
		return s;
	}
  	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof (on));
	if(srcip)
	{
		memset(&addr, 0, sizeof(struct sockaddr_in));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = srcip;
		addr.sin_port = 0;
		if(0 != bind(s,(struct sockaddr *)&addr,sizeof(addr)))
		{
			closesocket(s);
			return -1;
		}
	}
	memset(&to, 0, sizeof(struct sockaddr_in));
	to.sin_family = AF_INET;
	to.sin_port = htons((uint16_t)port);
	to.sin_addr.s_addr = ip;
	ret = connect(s, (struct sockaddr *)&to, sizeof(struct sockaddr_in));
	if(ret < 0)
	{
		closesocket(s);
		return -1;
	}
	bufsize = CFG_TRX_SIZE;
	setsockopt (s,SOL_SOCKET,SO_SNDBUF,(const char*)&bufsize, sizeof(bufsize));
	setsockopt (s,SOL_SOCKET,SO_RCVBUF,(const char*)&bufsize, sizeof(bufsize));
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 1000 * 1000;
        setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
        setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));
	return s;
}



int netCreateTCPConnect(unsigned short port)
{
	int s;
	struct sockaddr_in addr;
	int ret;
	int on = 1;
	s = socket(PF_INET, SOCK_STREAM, 0);
	if (s < 0) 
	{
		return -1;
	}
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof (on));
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	ret = bind(s,(struct sockaddr *)&addr,sizeof(addr));
	if (ret < 0) 
	{
		closesocket(s);
		return -1;
	}
	ret = listen(s, 256);
	if (ret < 0) 
	{
		closesocket(s);
		return -1;
	}
	int bufsize = CFG_TRX_SIZE;
	setsockopt (s,SOL_SOCKET,SO_SNDBUF,(const char*)&bufsize, sizeof(bufsize));
	setsockopt (s,SOL_SOCKET,SO_RCVBUF,(const char*)&bufsize, sizeof(bufsize));
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 500 * 1000;		
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));
	return s;
}


int netCreateUDPConnect(unsigned int local_ip,unsigned short local_port,unsigned int dst_ip,unsigned short dst_port)
{
    int ret;
    int s;
    struct sockaddr_in to;
    int on = 1;
    int bufsize;
    s = socket(AF_INET,SOCK_DGRAM, 0);
   if(s >= 0)
    {
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof (on));
        memset(&to, 0, sizeof(struct sockaddr_in));
        to.sin_family = AF_INET;
	to.sin_addr.s_addr = local_ip;
	to.sin_port = htons(local_port);
	if(0 != (ret = bind(s,(struct sockaddr *)&to,sizeof(to))))
	{
		closesocket(s);
		return -1;
	}
	if(dst_ip)
	{
	        to.sin_port = htons(dst_port);
	        to.sin_addr.s_addr = dst_ip;
	        if(0 != connect(s, (struct sockaddr *)&to, sizeof(struct sockaddr_in)))
	        {
              		closesocket(s);
   		        return -1;
        	}
	}
	bufsize = CFG_TRX_SIZE;
	setsockopt (s,SOL_SOCKET,SO_SNDBUF,(const char*)&bufsize, sizeof(bufsize));
	setsockopt (s,SOL_SOCKET,SO_RCVBUF,(const char*)&bufsize, sizeof(bufsize));
    }
    return s;
}


int netWait(int fd,int timeout)
{
	struct timeval t;
	int n;
	fd_set rd;
	if(fd < 0)
	{
		osSleep(100);
		return -1;
	}
	t.tv_sec = timeout / 1000;
	t.tv_usec = (timeout % 1000) * 1000;
	FD_ZERO(&rd);
	FD_SET(fd,&rd);
	n = select(fd + 1,&rd,NULL, NULL, &t);
	if(n < 0)
	{
		return -1;
	}	
	if(!FD_ISSET(fd,&rd))
	{
		return -1;
	}
	return 0;
}

int netWaitWR(int fd,int timeout)
{
	struct timeval t;
	int n;
	fd_set wr;
	if(fd < 0)
	{
		osSleep(100);
		return -1;
	}
	t.tv_sec = timeout / 1000;
	t.tv_usec = (timeout % 1000) * 1000;
	FD_ZERO(&wr);
	FD_SET(fd,&wr);
	n = select(fd + 1, NULL,&wr, NULL, &t);
	if(n < 0)
	{
		return -1;
	}	
	if(!FD_ISSET(fd,&wr))
	{
		return -1;
	}
	return 0;
}

int netReceive(int fd, uint8_t *buf, int len,int timeout)
{
	int flags = 0;
	int n;
	uint8_t* orig = buf;
	fd_set rd;
	struct timeval t;
	if(timeout < 0)
	{
		t.tv_sec = 0;
		t.tv_usec = 100 * 1000;
		FD_ZERO(&rd);
		FD_SET(fd,&rd);
		n = select(fd + 1, &rd,NULL, NULL, &t);
		if(n <= 0)
		{
			return -1;
		}
		return recv(fd,(char*)buf,len,flags);
	}
	while(len > 0)
	{
		t.tv_sec = timeout / 1000;
		t.tv_usec = (timeout % 1000) * 1000;
		FD_ZERO(&rd);
		FD_SET(fd,&rd);
		n = select(fd + 1, &rd,NULL, NULL, &t);
		if(n < 0)
		{
			return -1;
		}
		if(!FD_ISSET(fd,&rd))
		{
			return -1;
		}
		n = recv(fd, (char*)buf,len,flags);
		if(timeout < 0)
		{
			return n;
		}
		if(n < 0)
		{
			continue;
		}
		else if(n == 0)
		{
			return 0;
		}
		buf += n;
		len -= n;
	}
	return (unsigned int)buf - (unsigned int)orig;
}



int netSend(int fd, unsigned char *buf, int size, int timeout)
{
	fd_set writefds;
	struct timeval t;
	int n;
	int ret;
	int oldsize = size;
	if(fd < 0)
	{
		return -1;
	}
	do
	{
		if(timeout > 0)
		{
			t.tv_sec = timeout / 1000;
			t.tv_usec = (timeout % 1000) * 1000;
			FD_ZERO(&writefds);
			FD_SET(fd, &writefds);
			n = select(fd + 1, NULL, &writefds, NULL, &t);
			if ((n <= 0) || !FD_ISSET(fd,&writefds))
			{
				return -1;
			}
		}
		ret = send(fd,(char*)buf,size,0);
		if(ret > 0)
		{
			buf += ret;
			size -= ret;
		}
		else
		{
			if(errno == EAGAIN)
			{
				continue;
			}
			else if(errno == EPIPE)
			{
				return -1;
			}
			return -1;
		}
	}while(size > 0);
	return oldsize;
}


int netGetIPFrom(char* hostname)
{
        char msg[64];
        struct hostent* ent;
	int ret = inet_addr(hostname);
	if(ret && (ret != -1))
	{
		return ret;
	}
        if((ent = gethostbyname(hostname)) != NULL)
        {
                inet_ntop(ent->h_addrtype, ent->h_addr, msg, sizeof(msg));
                return inet_addr((char*)msg);
        }
        return -1;
}

static int runcmd(char* cmd,char* gw)
{
        char buf[256];
        FILE* p = popen(cmd,"r");
        if(!p)
        {
                return -1;
        }
        while(fgets(buf,256,p))
        {
		if(!strncmp(buf,"default",7))
		{
			sscanf(buf,"default        %[^ ]",gw);
			break;
		}
                osSleep(1);
        }
        return pclose(p);
}

int netGetGateway(char* gw)
{
	return runcmd("route",gw);
}


int netGetETHInfo(char *eth,int* ip,int* netmask,char* mac,int* link)
{
        int ret;
        int addrfd;
        struct  ifreq  ifr;
        int on = 1;
        strncpy(ifr.ifr_name, eth, IFNAMSIZ);
        addrfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (addrfd < 0)
        {
                return -1;
        }
        setsockopt(addrfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof (on));
	if(ip)
	{
		*ip = 0;
        	ret = ioctl(addrfd, SIOCGIFADDR, &ifr);
        	if (ret >= 0)
        	{
			memcpy(ip, &(((struct sockaddr_in*)(&ifr.ifr_addr))->sin_addr), 4);
		}
	}
	if(netmask)
	{
		*netmask = 0;
		if(ioctl(addrfd,SIOCGIFNETMASK,&ifr) >= 0)
		{
			memcpy(netmask, &(((struct sockaddr_in*)(&ifr.ifr_addr))->sin_addr), 4);
		}
	}
	if(mac)
	{
		if(ioctl(addrfd,SIOCGIFHWADDR,&ifr) >= 0)
		{
			sprintf(mac,"%02x:%02x:%02x:%02x:%02x:%02x",(unsigned char)ifr.ifr_hwaddr.sa_data[0],(unsigned char)ifr.ifr_hwaddr.sa_data[1],(unsigned char)ifr.ifr_hwaddr.sa_data[2],(unsigned char)ifr.ifr_hwaddr.sa_data[3],(unsigned char)ifr.ifr_hwaddr.sa_data[4],(unsigned char)ifr.ifr_hwaddr.sa_data[5]);
		}
	}
	if(link)
	{
		int ret;
		*link = 0;
		ret = detect_mii(addrfd,eth);
		*link = ret;
        }

        closesocket(addrfd);
        return 0;
}

int netParseURL(unsigned char* url,int defport,unsigned char* username,unsigned char* password,unsigned char* hostname,unsigned char* port,unsigned char* path,unsigned char* params)
{
    uint8_t* str = url + 7;
    uint8_t ch;
    char* unt,*pnt,*pt;
    *username = 0;
    *password = 0;
     str = (uint8_t*)strstr((char*)url,(const char*)"//");
    if(!str)
    {
      return -1;
     }
     str += 2;
    unt = strstr((char*)str,(const char*)":");
    pnt = strstr((char*)str,(const char*)"@");
    pt = strstr((char*)str,(const char*)"/");
    if(unt && pnt && pnt > unt)
    {
        if((pt > pnt) || !pt)
        {
            memcpy(username,str,(unsigned int)unt - (unsigned int)str);
            memcpy(password,unt + 1,(unsigned int)pnt - (unsigned int)unt - 1);
            str = (uint8_t*)pnt + 1;
        }
    }
    while(*str)
    {
        ch = *str;
        if((ch == ':') || (ch == '/'))
        {
                break;
        }
        else
        {
                *hostname ++= ch;
                str ++;
        }
    }
    *hostname = 0;
    if(*str != ':')
    {
        sprintf((char*)port,"%u",defport);
	if(*str)
	{
		str++;
	}
    }
    else
    {
	str++;
        while(*str)
        {
            ch = *str++;
            if(ch =='/')
            {
                   *port = 0;
                    break;
            }
            else
            {
                    *port ++= ch;
            }
        }
        *port = 0;
    }
    while(*str)
    {
    	if((*str == '?') || (*str <= 0x20))
    	{
		str ++;
    		break;
    	}
        ch = *str++;
        *path ++= ch;
    }
    *path = 0;
     while(*str)
     {
	*params = *str++;
	params++;
     }
     *params = 0;
    return 0;
}


int netGetLocalIP()
{
	int ret;
	int addrfd;
	struct  ifreq  ipaddrifr;
	//struct sockaddr_in *sin_ptr;
	int retsock;
	int on = 1;
	strncpy(ipaddrifr.ifr_name, "eth0", IFNAMSIZ); 
	addrfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (addrfd < 0) 
	{
		return -1;
	}
  	setsockopt(addrfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof (on));
	retsock = ioctl(addrfd, SIOCGIFADDR, &ipaddrifr);
	if (retsock < 0) 
	{
		closesocket(addrfd);
		return -1;
	}
	//sin_ptr = (struct sockaddr_in *)&ipaddrifr.ifr_addr;
	memcpy(&ret, &(((struct sockaddr_in*)(&ipaddrifr.ifr_addr))->sin_addr), 4);
	closesocket(addrfd);
	return ret;
}

int netTCPConnectTimeout(int ip, unsigned short int port, unsigned int timeout )
{
    struct sockaddr_in sin;
    int                s, flags, error_value, n;
    fd_set             rset, wset;
    struct timeval     tv;
    unsigned int       error_value_len;

    memset( &sin, 0, sizeof( sin ) );
    sin.sin_family      = AF_INET;
    sin.sin_addr.s_addr = ip;
    sin.sin_port        = htons( port );
    error_value         = 0;
    error_value_len     = sizeof( error_value );
    if ( ( s = socket( PF_INET, SOCK_STREAM, 0 ) ) < 0 )
    {
        return( -1 );
    }
    if ( ( flags = fcntl( s, F_GETFL, 0 ) ) < 0 )
    {
        goto tcp_connect_1;
    }
    if ( fcntl( s, F_SETFL, flags | O_NONBLOCK ) < 0 )
    {
        goto tcp_connect_1;
    }
    if ( ( n = connect( s, ( struct sockaddr * )&sin, sizeof( sin ) ) ) < 0 )
    {
        if ( errno != EINPROGRESS )
        {
            goto tcp_connect_1;
        }
    }
    if ( n == 0 )
    {
        goto tcp_connect_0;
    }
    FD_ZERO( &rset );
    FD_SET( s, &rset );
    wset       = rset;
    tv.tv_sec  = timeout;
    tv.tv_usec = 0;
    n = select( s + 1, &rset, &wset, NULL, timeout ? &tv : NULL );
    if ( n < 0 )
    {
        goto tcp_connect_1;
    }
    else if ( n == 0 )
    {
        goto tcp_connect_1;
    }
    else if ( FD_ISSET( s, &rset ) || FD_ISSET( s, &wset ) )
    {
        if ( getsockopt( s, SOL_SOCKET,SO_ERROR, &error_value,&error_value_len ) < 0 )
        {
            goto tcp_connect_1;
        }
    }
    else
    {
        goto tcp_connect_1;
    }

tcp_connect_0:

    if ( fcntl( s, F_SETFL, flags ) < 0 )
    {
        goto tcp_connect_1;
    }
    if ( error_value != 0 )
    {
        goto tcp_connect_1;
    }
    return( s );

tcp_connect_1:

    close( s );
    return( -1 );
}

int netTCPConnectTimeout2(int srcip,int ip, unsigned short int port, unsigned int timeout )
{
    struct sockaddr_in sin;
    int                s, flags, error_value, n;
    fd_set             rset, wset;
    struct timeval     tv;
    unsigned int       error_value_len;

    error_value         = 0;
    error_value_len     = sizeof( error_value );
    if ( ( s = socket( PF_INET, SOCK_STREAM, 0 ) ) < 0 )
    {
        return( -1 );
    }
    if(srcip)
    {
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = srcip;
	sin.sin_port = 0;
	if (bind (s, (struct sockaddr *) &sin, sizeof (sin)))
	{
		closesocket(s);
		return -1;
	}
    }
    memset( &sin, 0, sizeof( sin ) );
    sin.sin_family      = AF_INET;
    sin.sin_addr.s_addr = ip;
    sin.sin_port        = htons( port );
    if ( ( flags = fcntl( s, F_GETFL, 0 ) ) < 0 )
    {
        goto tcp_connect_1;
    }
    if ( fcntl( s, F_SETFL, flags | O_NONBLOCK ) < 0 )
    {
        goto tcp_connect_1;
    }
    if ( ( n = connect( s, ( struct sockaddr * )&sin, sizeof( sin ) ) ) < 0 )
    {
        if ( errno != EINPROGRESS )
        {
            goto tcp_connect_1;
        }
    }
    if ( n == 0 )
    {
        goto tcp_connect_0;
    }
    FD_ZERO( &rset );
    FD_SET( s, &rset );
    wset       = rset;
    tv.tv_sec  = timeout;
    tv.tv_usec = 0;
    n = select( s + 1, &rset, &wset, NULL, timeout ? &tv : NULL );
    if ( n < 0 )
    {
        goto tcp_connect_1;
    }
    else if ( n == 0 )
    {
        goto tcp_connect_1;
    }
    else if ( FD_ISSET( s, &rset ) || FD_ISSET( s, &wset ) )
    {
        if ( getsockopt( s, SOL_SOCKET,SO_ERROR, &error_value,&error_value_len ) < 0 )
        {
            goto tcp_connect_1;
        }
    }
    else
    {
        goto tcp_connect_1;
    }

tcp_connect_0:

    if ( fcntl( s, F_SETFL, flags ) < 0 )
    {
        goto tcp_connect_1;
    }
    if ( error_value != 0 )
    {
        goto tcp_connect_1;
    }
    return( s );

tcp_connect_1:
    close( s );
    return( -1 );
}


