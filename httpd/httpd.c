#include "httpd.h"

void print_log(const char* msg,int level)
{
#ifdef _STDOUT_
	const char * const level_msg[]=
	{
		"SUCCESS",
		"NOTICE",
		"WARNING",
		"ERROR",
		"FATAL",
	};
	printf("[%s][%s]\n",msg,level_msg[level%5]);

	
#endif
}

int startup(const char* ip,int port)
{
	//creat socket
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0)
	{
		print_log(strerror(errno), FATAL);
		exit(2);
	}	
	//fu yong socket,ke yi li ke chong qi
	int opt = 1;
	setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
	
	////bind
	struct sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	local.sin_addr.s_addr = inet_addr(ip);

	if(bind(sock,(struct sockaddr*)&local,sizeof(local))<0)
	{
		print_log(strerror(errno),FATAL);
		exit(3);
	}
	////listen_sock
	if(listen(sock,10) < 0){
		print_log(strerror(errno),FATAL);
		exit(4);
	}
	return sock;
}
/////////////////////////************///////////////////
//ret > 1, line != '\0',ret = 1&line = '\n', ret <=&&line=='\0'
int get_line(int sock,char line[],int size)
{
	//read 1 char,one by one
	char c = '\0';
	int len = 0;
	while(c != '\n' && len < size -1)
	{
		int r = recv(sock,&c,1,0);
		if(r > 0)
		{
			if(c == '\r'){
				//kui tan xia yi ge zi fu 
				int ret = recv(sock,&c,1,MSG_PEEK);
				if(ret > 0){
					if(c == '\n')
						recv(sock,&c,1,0);
					else
						c = '\n';
				}
			}
			//ci shi c ==\n
			line[len++] = c;
		}
		else
			c = '\n';
	}
	line[len] = '\0';
}	
static void echo_string(int sock)
{

}
///////////////wei xian shi xin xi ?????
//thread zhi xing
//chu li lian jie
void *hander_request(void * arg)
{
	int sock = (int)arg;
#ifdef _DEBUG_
	char line[1024];
	do
	{
		int ret = get_line(sock, line, sizeof(line));
		if(ret > 0){
			printf("%s",line);
		}
		else{
			printf("request......done\n");
			break;
		}
	}while(1);
#endif
//pan ding fang fa
	int ret = 0;
	char buf[SIZE];
	char method[SIZE/10];
	char url[SIZE];
	
	int i;
	int j;
	
	//ci chu she zhi CGI flag
	int cgi = 0;
	if(get_line(sock,buf,sizeof(buf)) <= 0){
		echo_string(sock);
		ret = 5;
		goto end;
	
	}
	i=0;//method ->index
	j=0;//buf ->index
	
	//GET / http/1.0
	while(!isspace(buf[j]) && i < sizeof(method) -1 && j<sizeof(buf)){
		method[i]=buf[j];
		i++;
		j++;
	}
	method[i] = 0;
	if(strcasecmp(method,"GET") && strcasecmp(method,"POST")){
		echo_string(sock);
		ret = 6;
		goto end;
	}
	//zhong jian kong ge ye xu hen duo
	//buf ->"GET   / http/1.0"
	if(isspace(buf[i]) && j < sizeof(buf))
	{
		j++;
	}
	i=0;
	//chu li qing qiu 
	while(!isspace(buf[j]) && j < sizeof(buf) && i < sizeof(url)-1)
	{
		url[i] = buf[j];
		i++;
		j++;
	}
	url[i] = 0;
	printf("method : %s, url: %s\n",method,url);






end:
	close(sock);
	return (void *)ret;
}

