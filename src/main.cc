#include <ssh-reverse-copy-id.h>
#include <iostream>

int main(int argc, char** argv)
{
  srci::Parser parser;
  parser.parse(argc, argv);
  if (parser.isValid()){
    srci::SshReverseCopyId sshReverseCopyId(parser.getUserName(), parser.getAddress());
    sshReverseCopyId.execute();
  }
  return 0;
}
