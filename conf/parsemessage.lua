--解析消息和发送send_message(fd, message)/send_message_all(message)
function parsemessage(fd, message)
   print(fd, message);
   send_message(fd, message);
end  