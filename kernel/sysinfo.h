struct sysinfo {
  uint64 freemem;      // số byte bộ nhớ trống
  uint64 nproc;        // số tiến trình không ở trạng thái UNUSED
  uint64 nopenfiles;   // số file đang mở trong toàn hệ thống
};
