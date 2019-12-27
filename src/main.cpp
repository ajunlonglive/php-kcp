#include "phpx.h"
#include "ikcp.h"
#include <iostream>

using namespace php;
using namespace std;

struct kcp_object
{
    ikcpcb *kcp;
    Variant *callback;
    Variant *resource;
};

void kcp_dtor(zend_resource *res)
{
    kcp_object *o = static_cast<kcp_object *>(res->ptr);
    ikcp_release(o->kcp);
    efree(o);
}

static int kcp_output(const char *buf, int len, ikcpcb *kcp, void *user)
{
    kcp_object *o = static_cast<kcp_object *>(user);
    Args array;
    array.append(*o->resource);
    Variant data(buf, len);
    array.append(data);

    Variant retval = call(*o->callback, array);
    return retval.toInt();
}

PHPX_FUNCTION(kcp_create)
{
    long conv = args[0].toInt();
    kcp_object *o = (kcp_object *) emalloc(sizeof(*o));
    ikcpcb *kcp = ikcp_create(conv, o);
    if (!kcp)
    {
        retval = false;
    }
    else
    {
        o->kcp = kcp;
        ikcp_nodelay(kcp, 0, 10, 0, 0);
        retval = newResource<kcp_object>("KCPObject", o);
    }
}

PHPX_FUNCTION(kcp_input)
{
    kcp_object *o = args[0].toResource<kcp_object>("KCPObject");
    auto data = args[1];
    retval = ikcp_input(o->kcp, data.toCString(), data.length());
}

PHPX_FUNCTION(kcp_flush)
{
    kcp_object *o = args[0].toResource<kcp_object>("KCPObject");
    ikcp_flush(o->kcp);
}

PHPX_FUNCTION(kcp_peeksize)
{
    kcp_object *o = args[0].toResource<kcp_object>("KCPObject");
    retval = ikcp_peeksize(o->kcp);
}

void itimeofday(long *sec, long *usec)
{
    struct timeval time;
    gettimeofday(&time, NULL);
    if (sec)
    {
        *sec = time.tv_sec;
    }
    if (usec)
    {
        *usec = time.tv_usec;
    }
}

uint64_t iclock64(void)
{
    long s, u;
    uint64_t value;
    itimeofday(&s, &u);
    value = ((uint64_t) s) * 1000 + (u / 1000);
    return value;
}

uint32_t iclock()
{
    return (uint32_t) (iclock64() & 0xfffffffful);
}

PHPX_FUNCTION(kcp_update)
{
    kcp_object *o = args[0].toResource<kcp_object>("KCPObject");
    uint32_t time1 = iclock();
    ikcp_update(o->kcp, time1);
}

PHPX_FUNCTION(kcp_recv)
{
    kcp_object *o = args[0].toResource<kcp_object>("KCPObject");
    size_t size = args[1].toInt();
    auto buf = zend_string_alloc(size + 1, 0);
    int n_read = ikcp_recv(o->kcp, buf->val, size);
    if (n_read <= 0)
    {
        retval = false;
    }
    else
    {
        buf->len = n_read;
        buf->val[buf->len] = 0;
        retval = buf;
    }
}

PHPX_FUNCTION(kcp_send)
{
    kcp_object *o = args[0].toResource<kcp_object>("KCPObject");
    auto data = args[1];
    retval = ikcp_send(o->kcp, data.toCString(), data.length());
}

PHPX_FUNCTION(kcp_setoutput)
{
    kcp_object *o = args[0].toResource<kcp_object>("KCPObject");
    auto cb = args[1];
    ikcp_setoutput(o->kcp, kcp_output);
    o->callback = cb.dup();
    o->resource = args[0].dup();
}

PHPX_EXTENSION()
{
    Extension *extension = new Extension("kcp", "0.0.1");

    extension->onStart = [extension]() noexcept {
        extension->registerConstant("KCP_VERSION", 10001);
        extension->registerResource("KCPObject", kcp_dtor);
    };

    //extension->onShutdown = [extension]() noexcept {
    //};

    //extension->onBeforeRequest = [extension]() noexcept {
    //    cout << extension->name << "beforeRequest" << endl;
    //};

    //extension->onAfterRequest = [extension]() noexcept {
    //    cout << extension->name << "afterRequest" << endl;
    //};

    extension->info({"kcp support", "enabled"},
                    {
                        {"version", extension->version},
                        {"date", "2019-12-27"},
                    });
    extension->registerFunction(PHPX_FN(kcp_create));
    extension->registerFunction(PHPX_FN(kcp_send));
    extension->registerFunction(PHPX_FN(kcp_recv));
    extension->registerFunction(PHPX_FN(kcp_update));
    extension->registerFunction(PHPX_FN(kcp_peeksize));
    extension->registerFunction(PHPX_FN(kcp_input));
    extension->registerFunction(PHPX_FN(kcp_setoutput));
    extension->registerFunction(PHPX_FN(kcp_flush));

    return extension;
}
