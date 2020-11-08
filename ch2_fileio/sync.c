/**
 * write 入内核缓冲, 不是磁盘
 * fsync(fd): 同步文件数据和元属性
 * fdatasync(fd): 同步文件数据
 * sync(): 同步所有文件
 * open(file, O_WRONLY | O_SYNC): 始终保持同步
 */