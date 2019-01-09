## 一个简单定时器队列框架  

### 特点   
- 单线程  
- 轻量级  
- 便于移植  
- 可在中断中调用
- 回调机制  

### API

- int equeue_create(equeue_t *equeue, int event_size)     
创建一个队列，用于执行定时器或者消息的回调，根据自己项目情况设置大小。    
- void equeue_run(equeue_t *equeue, equeue_tick_t ms)    
执行队列任务，正常执行不会返回。    
- int equeue_call(equeue_t *equeue, callback cb, void *obj, void *data)   
立即执行cb回调函数，参数为obj data。一般用在中断中，用于不想长时间占用中断，切换用户栈。  
- equeue_timer_t *equeue_call_in(equeue_t *equeue, equeue_tick_t ms, callback cb,
                               void *obj, void *data)   
单次在ms毫秒后执行cb回调函数，参数为obj和data。   
- equeue_timer_t *equeue_call_every(equeue_t *equeue, equeue_tick_t ms,
                                  callback cb, void *obj, void *data)   
每隔ms毫秒执行cb回调函数，参数为obj和data。   
- int equeue_call_cancel(equeue_t *equeue, equeue_timer_t *timer)  
用于取消某个定时器，常用于超时处理。比如串口接收超时定时器，已经接收到数据进行定时器取消。
- int equeue_call_restart(equeue_t *equeue, equeue_timer_t *timer, int ms)  
重新设置定时器。  
- int equeue_add_listener(equeue_t *equeue, const char *name, callback cb,
                        void *obj)   
  监听name消息，接收消息后执行cb回调函数。
- int equeue_remove_listener(equeue_t *equeue, const char *name)  
移除对name消息的监听。   
- int equeue_dispatch_event(equeue_t *equeue, const char *name, void *data)  
发送name 消息。用于解耦发起者和执行者。

### 测试  

```
git clone git@github.com:lihaichen/equeue.git  
make 
make test 
./test

```

### 移植

参考 equeue_config.h equeue_no_os.c




