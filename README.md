# malloqueiro
Simple malloc/free implementation in x86 assembly

```shell
sudo apt install gcc-multilib g++-multilib libc6-dev-i386

make clean \
  && make all \
  && make -C test/ clean \
  && make -C test/ all \
  && make -C test/ run_tests
```
