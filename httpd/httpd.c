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
static int get_line(int sock,char line[],int size)
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
	return len;
}	
static void echo_string(int sock)
{

}
/////////////////////////////////////////////////

static int echo_www(int sock,char* path,int size)
{
	int fd = open(path,O_RDONLY);
	if(fd < 0)
	{
		echo_string(sock);
		print_log(strerror(errno), FATAL);
		return 8;
	}
	
	const char* echo_line = "HTTP/1.0 200 OK\r\n";
	send(sock,echo_line,strlen(echo_line),0);
	const char* null_line = "\r\n";
	send(sock,echo_line,strlen(echo_line),0);

	if(sendfile(sock,fd,NULL,size)<0)
	{
		echo_string(sock);
		print_log(strerror(errno),FATAL);
		return 9;
	}
	close(fd);
	return 0;
}
static void drop_header(int sock)
{	
	char line[1024];
	int ret = -1;
	do{
		ret = get_line(sock, line,sizeof(line));
	}while(ret > 0 && strcmp(line,"\n"));
}
static int exe_cgi(int sock,char* method,char* path,char* query_string)
{
	int contnet_len = -1;
	if(strcasecmp(method,"GET")==0)
	{
		drop_header(sock);
	}
	else{//post
		char line[1024];
		int ret = -1;
		do{
			ret = get_line(sock,line,sizeof(line));
			if(ret > 0 && (strncmp(line,"Content-Length: ",16)==0))
			{
				contnet_len = atoi(&line[16]);
			}
		}while(ret > 0&&strcmp(line,"\n"));
		pid_t pid = fork();
		if(pid < 0){
			echo_string(sock);
			return 11;
		}
		else if(pid == 0)//child
		{
			execl(path,path,NULL);
			exit(1);
		}	
		else{//father
			int ret = waitpid(pid,NULL,0);
		}
		
	}
}
////////////////////////////////////////////////
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
#else
//pan ding fang fa
	int ret = 0;
	char buf[SIZE];
	char method[SIZE/10];
	char url[SIZE];
	char path[SIZE];
	
	int i;
	int j;
	
	//ci chu she zhi CGI flag
	int cgi = 0;
	char *query_string = NULL;//qing qiu zi fu chuan
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

	/////////////////////////////////////////////
	//chu li GET fang fa zhong ? zi fu
	query_string = url;
	while(*query_string != '\0'){
		if(*query_string =='?'){
			*query_string = '\0';
			query_string++;
			cgi = 1;
			break;
		}
		query_string++;
	}
	//ping jie
	sprintf(path,"wwwroot%s",url);
	if(path[strlen(path)-1] == '/')
	{
		strcat(path,"index.html");
	}

	struct stat st;
	if(stat(path,&st)!=0)
	{	
		echo_string(sock);
		ret = 7;
		goto end;
	}
	else{
		if(S_ISDIR(st.st_mode))
		{
			strcat(path,"/index.html");
		}
		else if((st.st_mode & S_IXUSR)||\
				(st.st_mode & S_IXGRP)||\
				(st.st_mode & S_IXOTH))
		{
			cgi = 1;
		}
		else{
			
		}
		if(cgi)
		{
			exe_cgi(sock,method,path,query_string);
		}
		else{
			printf("method : %s ,url :%s, path: %s,cgi: %d, query_string: %s\n",method,url,path,cgi,query_string);
			drop_header(sock);
			echo_www(sock,path,st.st_size);
		}
	}
end:
	printf("quit client\n");
	close(sock);
	return (void *)ret;
#endif
}

