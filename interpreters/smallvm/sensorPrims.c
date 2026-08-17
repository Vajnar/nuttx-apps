#include <nuttx/config.h>
#include <nuttx/i2c/i2c_master.h>
#include <sys/ioctl.h>


static int master_read_i2c(int fd, uint8_t addr, uint8_t* buffer, int len) {
  struct i2c_transfer_s i2c_transfer;
  struct i2c_msg_s i2c_msg[1];
  int ret;
  int i;

#ifdef CONFIG_I2C_SLAVE_WRITEBUFSIZE
  if (len > CONFIG_I2C_SLAVE_WRITEBUFSIZE) {
      printf("buffer size %d bigger than slave_writebufsize %d\n", \
      len, CONFIG_I2C_SLAVE_WRITEBUFSIZE);
      return -1;
  }
#endif

  i2c_msg[0].addr = addr;
  i2c_msg[0].flags = 1;
  i2c_msg[0].buffer = buffer;
  i2c_msg[0].length = len;
  i2c_msg[0].frequency = 400000;

  i2c_transfer.msgv = (struct i2c_msg_s *)i2c_msg;
  i2c_transfer.msgc = 1;

  ret = ioctl(fd, I2CIOC_TRANSFER, (unsigned long)&i2c_transfer);
  if (ret < 0) {
      printf("read_i2c failed\n");
  }
  return ret;
}

static int master_write_i2c(int fd, uint8_t addr, uint8_t* buffer, int len) {
  struct i2c_transfer_s i2c_transfer;
  struct i2c_msg_s i2c_msg[1];
  int ret;
  int i;

#ifdef CONFIG_I2C_SLAVE_WRITEBUFSIZE
  if (len > CONFIG_I2C_SLAVE_WRITEBUFSIZE) {
      printf("buffer size %d bigger than slave_writebufsize %d\n", \
      len, CONFIG_I2C_SLAVE_WRITEBUFSIZE);
      return -1;
  }
#endif

  i2c_msg[0].addr = addr;
  i2c_msg[0].flags = 0;
  i2c_msg[0].buffer = buffer;
  i2c_msg[0].length = len;
  i2c_msg[0].frequency = 400000;

  i2c_transfer.msgv = (struct i2c_msg_s *)i2c_msg;
  i2c_transfer.msgc = 1;

  ret = ioctl(fd, I2CIOC_TRANSFER, (unsigned long)&i2c_transfer);
  if (ret < 0) {
      printf("write_i2c failed\n");
  }

  return ret;
}
