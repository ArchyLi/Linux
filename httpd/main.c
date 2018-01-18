#include "httpd.h"
#include <stdio.h>
#include <pthread.h>
static void usage(const char * proc)
{
	printf("usage: %s,[ip][prot]\n",proc);
}

int main(int argc, char* argv[])
{
	if(argc != 3)
	{
		usage(argv[0]);
		return 1;
	}
	int listen_sock = startup(argv[1],atoi(argv[2]));
	while(1)
	{
		struct sockaddr_in client;
		socklen_t len = sizeof(client);

		int new_sock = accept(listen_sock,(struct sockaddr*)&client,&len);
		if(new_sock < 0)
		{
			print_log(strerror(errno),NOTICE);
			continue;
		}
		
		printf("get client[%s:%d]\n",inet_ntoa(client.sin_addr),ntohs(client.sin_port));
		//fu xian cheng deng dai ,zi xian cheng zhi xing zhu shi jian 
		pthread_t id;
		int ret = pthread_create(&id, NULL,hander_request ,(void *)new_sock);
		if(ret != 0){
			print_log(strerror(errno),WARNING);
			close(new_sock);
		}else{
			pthread_detach(id);
		}
	}
	//bu hui zhi xing
	close(listen_sock);
	return 0;
}
