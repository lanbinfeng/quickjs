import time
from quickjs import Context

def py_print(name):
    print(name)

def py_sleep():
    time.sleep(1)

py_sleep()

# 创建QuickJS上下文
ctx = Context()
print(ctx)
ctx.debugger_wait('127.0.0.1:9999')
ctx.add_callable( "py_print", py_print)
ctx.add_callable( "py_sleep", py_sleep)

code = """
function boops() {
	py_print('hello!');
}
const gg = 9;
function foo (t) {
    var a = 55;
    var b = 33;
    var c = {
        d: true,
        e: 'hello',
        f: 34.55,
    };

    var arr2 = new Uint8Array(10000);
    var arr = [];
    for (var i = 0; i < 10000; i++) {
        arr.push(i);
        arr2[i] = i;
    }

    function noob() {
        py_print('f;asdsad`')
        py_print(a);
        py_print(t);
        py_print('supsups')
        py_print('ubgasdsad')
    }
    noob();
}

function bar() {
    foo(3);
    py_print('asdsad');
    py_print('about to throw!');
    
    let index = 0;
    while(true)
    {
        py_print("123:" + index);
        py_sleep();
        ++index;
    };
    
    
    try {
        throw new Error('whoops');
    }
    catch (e) {
        py_print('caught a whoops');
    }
}

class Blub {
    constructor() {
        this.peeps = 3;
    }
    jib() {
        py_print(this);
        bar();
    }
}

var blub = new Blub();
blub.jib();
// note that a breakpoint here will not work, tail call breakpoints fail?
blub.jib();
"""

# 在JavaScript中调用注册的函数
ctx.eval(code)
