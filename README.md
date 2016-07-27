# DNS resolver
Асинхронный DNS клиент, представляет из себя компиляцию библиотек [c-ares] и [libev]. 

c-ares реализует механизм select() для неблокирующего ввода/вывода, libev в свою очередь расширяет этот список до /dev/poll, kqueue, POSIX select(), Windows select(), poll(), и epoll().

```bash
Usage: dns_resolver [KEY]... DOMAIN-LIST

	-n	number asynchronous requests
	-o	output file found domains
	-c	continue with the last entry of the output file

Example:
$ dns_resolver -n 50 -c -o domains_ip.csv domains.csv
```

В данной реализации список доменов должен представлять из себя CSV файл, где первым значением идет имя домена.
```
c0dedgarik.blogspot.ru;
```

Если вы хотите запустить процесс, как демон не привязанный к терминалу:
```bash
$ setsid cmd >/dev/null 2>&1
```
##Installation
```bash
$ make
```

### libev
Библиотека для упрощения и унификации поддержки механизма асинхронного неблокирующего ввода/вывода и механизма оповещения о событиях с помощью выполнения обратного вызова (callback) функции при наступлении заданного события для дескриптора файла или при достижении заданного таймаута (timeout). 

### c-ares
Библиотека для асинхроных запросов DNS.

[libev]: http://software.schmorp.de/pkg/libev.html
[c-ares]: http://c-ares.haxx.se/
