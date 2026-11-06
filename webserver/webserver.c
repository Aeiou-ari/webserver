#include "head.h"
#include "sqlite3.h"

const char *resp_404 = "HTTP/1.1 404 Not Found\r\nContent-Length:0\r\n\r\n";

typedef struct HttpRequest
{
    char *pmethod;
    char *purl;
    char *pcontent;
}HttpRequest_t;

typedef struct TmpData
{
    char *pname;
    char *ppwd;
    int flag;
}data_t;

//url解码函数，把%E4%B8%89这种转成中文
void url_decode(char *dst, const char *src)
{
    char a,b;
    while(*src)
    {
        if((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b)))
        {
            if(a>='a') a -= 'a'-10;
            else if(a>='A') a -= 'A'-10;
            else a -= '0';

            if(b>='a') b -= 'a'-10;
            else if(b>='A') b -= 'A'-10;
            else b -= '0';

            *dst++ = (a<<4)+b;
            src +=3;
        }
        else if(*src == '+')
        {
            *dst++ = ' ';
            src++;
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

int CreateUserDB(const char *pDBName)
{
    sqlite3 *pdb=NULL;
    char cmd[256]={0};
    char *perrmsg=NULL;
    int ret=0;

    ret=sqlite3_open(pDBName, &pdb);
    if (ret != SQLITE_OK)
    {
        fprintf(stderr,"sqlite3_open failed:%s\n",sqlite3_errmsg(pdb));
        return -1;
    }
   sprintf(cmd, "create table if not exists user_info(id integer primary key autoincrement,username text,password text);");
    ret=sqlite3_exec(pdb, cmd, NULL,NULL,&perrmsg);
    if (ret != SQLITE_OK)
    {
        fprintf(stderr,"sqlite3_exec failed:%s\n",perrmsg);
        return -1;
    }
    sqlite3_close(pdb);
    return 0;
}
int User_callback(void *arg,int colnum,char **pcontent,char **ptitle)
{
    data_t *pdata=arg;
    pdata->flag=1;
    return 0;
}
int UserRegister(const char *pDBName, char *pname, char *ppwd)
{
    int ret=0;
    sqlite3 *pdb=NULL;
    char  cmd[1024]={0};
    char *perrmsg=NULL;

    data_t arg;
    arg.flag=0;//初始0，用户不存在

    ret=sqlite3_open(pDBName, &pdb);
    if (ret != SQLITE_OK)
    {
        fprintf(stderr, "sqlite3_open failed:%s\n",sqlite3_errmsg(pdb));
        return -1;
    }
    sprintf(cmd, "select  password from user_info where username=\"%s\";",pname);
    ret=sqlite3_exec(pdb,cmd,User_callback,&arg,&perrmsg);
    if (ret != SQLITE_OK)
    {
        fprintf(stderr, "sqlite3_exec failed:%s\n",perrmsg);
        sqlite3_free(perrmsg);
        sqlite3_close(pdb);
        return -1;
    }

    //判断用户名是否存在
    if (arg.flag==1)
    {
        sqlite3_close(pdb);//用户存在
        return 1;
    }
    sprintf(cmd, "insert into user_info(username,password) values(\"%s\",\"%s\");",pname,ppwd);
    ret=sqlite3_exec(pdb,cmd,NULL,NULL,&perrmsg);
    if (ret != SQLITE_OK)
    {
        fprintf(stderr, "sqlite3_exec failed:%s\n",perrmsg);
        return -1;
    }
    sqlite3_close(pdb);
    return 0;
}

int handle_register(HttpRequest_t *pTmpRequest,const char * pfilename)
{
    char *body=NULL;
    char *pname=NULL;
    char *ppassword=NULL;
    char *prepwd=NULL;
    FILE *fp=NULL;
    int ret=0;

    body=pTmpRequest->pcontent+4;
    strtok(body,"=");//裁掉username
    pname=strtok(NULL, "&");//拿到名字

    strtok(NULL, "=");
    ppassword=strtok(NULL, "&");

    strtok(NULL, "=");
    prepwd=strtok(NULL,"\0");

printf("username:%s pwd:%s rpwd:%s\n",pname,ppassword,prepwd);
    fp=fopen(pfilename,"w");
    if (NULL==fp)
    {
        perror("fail to fopen");
        return -1;
    }
     fprintf(fp, "<!DOCTYPE html>\
                        <html>\
                        <head>\
                        <meta charset=\"utf-8\">\
                    <title>注册结果</title>\
                    </head>\
                    <body>");
    if (NULL==pname || NULL==ppassword || NULL==prepwd)
    {
        fprintf(fp,"用户名/密码不能为空");
    }
    else if (strcmp(ppassword, prepwd)!=0)
    {
        fprintf(fp, "请输入相同的密码 <a href = \"/register.html\">返回注册</a>");
    }
    else
    {
        ret=UserRegister("user.db",pname,ppassword);
        if (ret==0)
        {
            fprintf(fp, "注册成功! <a href = \"/index.html\">前往登录</a>");
        }
        else if (1 == ret)
        {
            fprintf(fp, "注册失败:用户名已存在<a href = \"/register.html\">返回注册</a>");
        }
        else
        {
            fprintf(fp,"诶呦卡住了,请稍后重试~");
        }
    }

    fprintf(fp, "</body></html>");
    fclose(fp);
return 0;
}
 int goods_callback(void *arg, int colnum, char **pcontent, char **ptitle)
{
    FILE *fp = (FILE *)arg;

    fprintf(fp,
"<div style='border:1px #eee solid;padding:10px;margin:8px;border-radius:12px;max-width:240px;display:inline-block;vertical-align:top;'>\n"
    );

    if (pcontent[4] != NULL && strlen(pcontent[4]) > 0)
    {
        fprintf(fp, "<a href='/detail?id=%s'>\n", pcontent[0]);
        fprintf(fp, "<img src='/goods_img/%s' style='width:100%%;height:160px;object-fit:cover;border-radius:8px;' />\n", pcontent[4]);
        fprintf(fp, "</a>\n");
    }
    else
    {
        fprintf(fp, "<div style='width:100%%;height:160px;background:#eee;border-radius:8px;display:flex;align-items:center;justify-content:center'>暂无图片</div>\n");
    }

    fprintf(fp, "<p>商品ID:%s</p>\n", pcontent[0]);
    fprintf(fp, "<p>商品名称:%s</p>\n", pcontent[1]);
    fprintf(fp, "<p>售价:%s 元</p>\n", pcontent[2]);
    fprintf(fp, "<p>库存:%s 件</p>\n", pcontent[3]);
    fprintf(fp, "</div>\n");

    return 0;
}

int GoodSearch(const char *pDbName, char *keyword, FILE *fp)
{
    sqlite3 *pdb = NULL;
    char cmd[512]={0};
    char *perrmsg = NULL;
    int ret;

    ret = sqlite3_open(pDbName, &pdb);
    if(ret != SQLITE_OK)
    {
        fprintf(stderr,"sqlite open fail:%s\n",sqlite3_errmsg(pdb));
        return -1;
    }

    // 同时匹配商品名+keywords介绍
   sprintf(cmd,"select goods_id,goods_name,shop_price,goods_number,original_img,goods_desc from goods where goods_name like '%%%s%%' or keywords like '%%%s%%';",keyword,keyword);
    // arg直接传文件指针fp！回调里面直接输出html，中间不经过C结构体数组
    ret = sqlite3_exec(pdb, cmd, goods_callback, fp, &perrmsg);
    if(ret != SQLITE_OK)
    {
        fprintf(stderr,"exec err:%s\n",perrmsg);
        sqlite3_free(perrmsg);
        sqlite3_close(pdb);
        return -1;
    }

    sqlite3_close(pdb);
    return 0;
}



/*解析报文，strtstr，strtok，判断搜索信息与数据表里信息是否一致，看东西有没有*/

int handle_search(HttpRequest_t *pTmpRequest,const char * pfilename)
{
    char *body=NULL;
    char *pgood=NULL;
    FILE *fp=NULL;
    int ret=0;
    int count=0;
    char keyword[256]={0};

    //body=pTmpRequest->pcontent+4;
    body=pTmpRequest->pcontent;
    strtok(body,"=");
    pgood=strtok(NULL,"\0");
    url_decode(keyword,pgood);
   fp=fopen(pfilename, "w");
   if (NULL==fp)
   {
    perror("fail to open");
    return -1;
   }
   fprintf(fp, "<html><body><h2>搜索结果:%s</h2>",keyword);
   ret=GoodSearch("123.db", keyword,fp);
   if (0==ret)
   {
    fprintf(fp,"<p>为您找到宝贝啦!</p>");
   }
   else
   {
    fprintf(fp, "<p>诶呦出错啦~</p>");
   }
   fprintf(fp, "</body></html>");
   fclose(fp);
    return 0;
}
/*解析url里的id参数*/
int get_query_id(char *url, char *id_buf, int buf_len)
{
    char *p = strchr(url, '?');
    if (!p) return -1;

    p++;

    char key[32] = {0};
    char val[32] = {0};

    sscanf(p, "%[^=]=%s", key, val);

    if (strcmp(key, "id") != 0)
    {
        return -1;
    }

    strncpy(id_buf, val, buf_len - 1);
    id_buf[buf_len - 1] = '\0';

    return 0;
}
/*商品详情页回调*/
int detail_callback(void *arg, int colnum, char **pcontent, char **ptitle)
{
    FILE *fp = (FILE *)arg;

    fprintf(fp,
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"<meta charset='utf-8'>\n"
"<title>商品详情</title>\n"
"<style>\n"
"body { background:#f8f0f4; padding:30px; font-family:Microsoft Yahei; }\n"
".detail { max-width:600px; margin:0 auto; background:#fff; padding:24px; border-radius:12px; }\n"
"img { width:100%%; max-width:400px; border-radius:8px; }\n"
"h2 { color:#d8789e; }\n"
".price { color:#e64378; font-size:22px; font-weight:bold; }\n"
".back { margin-top:20px; display:inline-block; padding:10px 20px; background:#ec407a; color:#fff; border-radius:8px; text-decoration:none; }\n"
"</style>\n"
"</head>\n"
"<body>\n"
"<div class='detail'>\n"
    );

    if (pcontent[4] != NULL && strlen(pcontent[4]) > 0)
    {
        fprintf(fp, "<img src='/goods_img/%s' alt='%s'>\n", pcontent[4], pcontent[1]);
    }
    else
    {
        fprintf(fp, "<div style='width:100%%;height:200px;background:#eee;border-radius:8px;display:flex;align-items:center;justify-content:center'>暂无图片</div>\n");
    }

    fprintf(fp, "<h2>%s</h2>\n", pcontent[1]);
    fprintf(fp, "<p>商品ID:%s</p>\n", pcontent[0]);
    fprintf(fp, "<p class='price'>¥%s</p>\n", pcontent[2]);
    fprintf(fp, "<p>库存：%s 件</p>\n", pcontent[3]);

    if (pcontent[5] != NULL && strlen(pcontent[5]) > 0)
    {
        fprintf(fp, "<p>商品介绍：%s</p>\n", pcontent[5]);
    }
    else
    {
        fprintf(fp, "<p>商品介绍：这是一款品质不错的数码配件，实用性强，价格实惠。</p>\n");
    }

    fprintf(fp, "<a class='back' href='/search.html'>返回搜索</a>\n");
    fprintf(fp, "</div>\n</body>\n</html>\n");

    return 0;
}
/*处理详情页请求*/
int handle_detail(int confd, char *goods_id)
{
    sqlite3 *pdb = NULL;
    char sql[512] = {0};
    char *errmsg = NULL;

    char buf[8192] = {0};
    FILE *fp = fmemopen(buf, sizeof(buf), "w");
    if (!fp)
    {
        send(confd, resp_404, strlen(resp_404), 0);
        return -1;
    }

    int ret = sqlite3_open("123.db", &pdb);
    if (ret != SQLITE_OK)
    {
        fprintf(fp, "<html><body>数据库打开失败</body></html>");
    }
    else
    {
        sprintf(sql,
            "SELECT goods_id,goods_name,shop_price,goods_number,original_img,goods_desc "
            "FROM goods WHERE goods_id = %s;",
            goods_id
        );

        sqlite3_exec(pdb, sql, detail_callback, fp, &errmsg);

        if (errmsg != NULL)
        {
            fprintf(fp, "<html><body>查询商品详情失败</body></html>");
            sqlite3_free(errmsg);
        }

        sqlite3_close(pdb);
    }

    fclose(fp);

    char header[512] = {0};
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %zu\r\n"
        "\r\n",
        strlen(buf)
    );

    send(confd, header, strlen(header), 0);
    send(confd, buf, strlen(buf), 0);

    return 0;
}


int CreateListenSocket(int Port)
{
    int ret = 0;
    int sockfd = 0;
    struct sockaddr_in seraddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd)
    {
        perror("fail to socket");
        return -1;
    }
    
    seraddr.sin_family = AF_INET;
    seraddr.sin_port = htons(Port);
    seraddr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(sockfd, (struct sockaddr *)&seraddr, sizeof(seraddr));
    if (-1 == ret)
    {
        perror("fail to bind");
        return -1;
    }

    ret = listen(sockfd, 10);
    if (-1 == ret)
    {
        perror("fail to listen");
        return -1;
    }

    return sockfd;
}

int RecvHttpRequest(int confd, char *precvbuf, int maxlen)
{
    ssize_t nret = 0;

    nret = recv(confd, precvbuf, maxlen, 0);
    if (-1 == nret)
    {
        perror("fail to recv");
        return -1;
    }

    return nret;
}

int SendHttpResponeHeader(int confd,char *pfilename)
{
    char tmpbuff[4096] = {0};
    ssize_t nret = 0;

    if (0 == access(pfilename, F_OK))
    {
        memset(tmpbuff, 0, sizeof(tmpbuff));
        sprintf(tmpbuff, "HTTP/1.1 200 OK\r\n");
        sprintf(tmpbuff, "%sServer: TinyServer\r\n", tmpbuff);
        sprintf(tmpbuff, "%sContent-Type: text/html; charset=utf-8;\r\n", tmpbuff);
        sprintf(tmpbuff, "%sConnection: keep-alive\r\n", tmpbuff);
        sprintf(tmpbuff, "%s\r\n", tmpbuff);
    }
    
    else
    {
        memset(tmpbuff, 0, sizeof(tmpbuff));
        sprintf(tmpbuff, "HTTP/1.1 404 NotFound\r\n");
        sprintf(tmpbuff, "%sServer: TinyServer\r\n", tmpbuff);
        sprintf(tmpbuff, "%sContent-Type: text/html; charset=utf-8;\r\n", tmpbuff);
        sprintf(tmpbuff, "%sConnection: keep-close\r\n", tmpbuff);
        sprintf(tmpbuff, "%s\r\n", tmpbuff);
    }

    nret = send(confd, tmpbuff, strlen(tmpbuff), 0);
    if (-1 == nret)
    {
        perror("fail to send");
        return -1;
    }

    return 0;
}
int SendHttpResponeContent(int confd, char *pfilename)
{
    int fd = 0;
    char tmpbuff[4096] = {0};
    ssize_t nret = 0;

    fd = open(pfilename, O_RDONLY);
    if (-1 == fd)
    {
        perror("fail to open");
        return -1;
    }

    while (1)
    {
        memset(tmpbuff, 0, sizeof(tmpbuff));
        nret = read(fd, tmpbuff, sizeof(tmpbuff));
        if (-1 == nret)
        {
            perror("fail to read");
            return -1;
        }
        else if (0 == nret)
        {
            break;
        }

        nret = send(confd, tmpbuff, nret, 0);
        if (-1 == nret)
        {
            perror("fail to send");
            return -1;
        }
    }

    close(fd);

    return 0;
}

int SendHttpRespone(int confd, HttpRequest_t *pTmpRequest)
{
    char tmpbuff[4096] = {0};
    char filename[256] = {0};
    int ret=0;

    if (0 == strcmp(pTmpRequest->pmethod, "GET"))
    {
        // 处理商品图片
    if (strstr(pTmpRequest->purl, "/goods_img/") == pTmpRequest->purl)
    {
        char full_path[256];
        strcpy(full_path, "./goods_img/");
        strcat(full_path, pTmpRequest->purl + strlen("/goods_img/"));

        FILE *fp = fopen(full_path, "rb");
        if (fp == NULL)
        {
            send(confd, resp_404, strlen(resp_404), 0);
            return 0;
        }

        char img_header[256];
        snprintf(img_header, sizeof(img_header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: image/jpeg\r\n"
            "\r\n"
        );
        send(confd, img_header, strlen(img_header), 0);

        unsigned char tmp[4096];
        int n;
        while ((n = fread(tmp, 1, sizeof(tmp), fp)) > 0)
        {
            send(confd, tmp, n, 0);
        }

        fclose(fp);
        return 0;
    }

    // 处理商品详情页
    if (strstr(pTmpRequest->purl, "/detail") == pTmpRequest->purl)
    {
        char id_buf[64] = {0};
        if (get_query_id(pTmpRequest->purl, id_buf, sizeof(id_buf)) == 0)
        {
            handle_detail(confd, id_buf);
            return 0;
        }
        else
        {
            send(confd, resp_404, strlen(resp_404), 0);
            return 0;
        }
    }
        if (0 == strcmp(pTmpRequest->purl, "/"))
        {
            sprintf(filename, "./index.html");  
        }
        else if (0 == strcmp(pTmpRequest->purl, "/favicon.ico"))
        {
            return 0;
        }
        else 
        {
            sprintf(filename, ".%s", pTmpRequest->purl);
        }
        // GET处理完成直接走发送，不要再往下执行POST逻辑！
        SendHttpResponeHeader(confd, filename);
        SendHttpResponeContent(confd, filename);
        return 0;   // 直接return，阻止进入下面POST的if‑else
    }

    // 只有POST才走到这里
    if(0==strcmp(pTmpRequest->pmethod, "POST"))
    {
        if (0==strcmp(pTmpRequest->purl,"/register"))
        {
            ret=handle_register(pTmpRequest,"./reg_temp.html");
            if (0 == ret)
            {
                sprintf(filename, "./reg_temp.html");
            }
            else 
            {
                sprintf(filename, "./register.html");
            }
        }
        else if (0 == strcmp(pTmpRequest->purl, "/login"))   
        {
           sprintf(filename, "./search.html");
        }
        else if (0 == strcmp(pTmpRequest->purl, "/search"))
        {
            ret=handle_search(pTmpRequest, "./goods_tmp.html");
            if (0==ret)
            {
                sprintf(filename, "./goods_tmp.html");
            }
            else
            {
                sprintf(filename,"./search.html");
            }
        }
        else  
        {
            sprintf(filename, "./index.html");
        }
        SendHttpResponeHeader(confd, filename);
        SendHttpResponeContent(confd, filename);
        return 0;
    }

    // 其他未知方法，返回404
    sprintf(filename,"./none.html");
    SendHttpResponeHeader(confd, filename);
    SendHttpResponeContent(confd, filename);
    return 0;
}


int PraseHttpRequest(char *precvbuf, HttpRequest_t *pTmpRequest)
{
    pTmpRequest->pcontent=strstr(precvbuf, "\r\n\r\n");
    pTmpRequest->pmethod = strtok(precvbuf, " ");
    pTmpRequest->purl = strtok(NULL, " ");

    return 0;
}

int main(void)
{
    int sockfd = 0;
    int confd = 0;
    char recvbuf[4096] = {0};
    HttpRequest_t TmpRequest;

    CreateUserDB("user.db");
    sockfd = CreateListenSocket(8080);

    while (1)
    {
        confd = accept(sockfd, NULL, NULL);
        if (-1 == confd)
        {
            perror("fail to accept");
            return -1;
        }
        memset(recvbuf,0,sizeof(recvbuf));
        RecvHttpRequest(confd, recvbuf, sizeof(recvbuf));

        printf("============================== RECV ==============================\n");
        printf("%s\n", recvbuf);
        printf("==================================================================\n");

        PraseHttpRequest(recvbuf, &TmpRequest);

        SendHttpRespone(confd, &TmpRequest);

        close(confd);
    }
    
    close(sockfd);

    return 0;
}