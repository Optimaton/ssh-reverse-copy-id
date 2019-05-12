#ifndef __SSH_REVERSE_COPY_ID_H__
#define __SSH_REVERSE_COPY_ID_H__
#include <iostream>
#include <cstdio>
#include <cstring>
#include <curl/curl.h>
#include <arpa/inet.h>
#include <termios.h>
#include <unistd.h>

/* Refer : https://curl.haxx.se/libcurl/c/sftpget.html 
   To switch off ssh-agent in this program */
#undef DISABLE_SSH_AGENT

namespace srci {

enum class UserError {
  INVALID_ARG_COUNT,
  INVALID_CMD_NAME,
  INVALID_ADDR_STRING,
  ILLFORMED_ARG,
};


void handleError(UserError userError) 
{
  switch(userError) {
    case UserError::INVALID_ARG_COUNT: std::cout << "Error: Invalid Argument Count.\n"; break;
    case UserError::INVALID_CMD_NAME: std::cout << "Error:Invalid Command Name. \n";break;
    case UserError::INVALID_ADDR_STRING: std::cout << "Error: Invalid Address String. \n"; break;
    case UserError::ILLFORMED_ARG: std::cout << "Error: Illformed Argument. \n"; break;
  }
}


class Parser {
public:
  Parser() = default;
  void usage() 
  {
    std::cout << "Usage: ssh-reverse-copy-id user@address" << std::endl;
  }


  /* */
  void parse(int argc, char** argv) 
  {
    if (argc != MIN_ARG_CNT) {
      handleError(UserError::INVALID_ARG_COUNT);
      usage();
      std::exit(EXIT_FAILURE);
    }
    
    if (strcmp(argv[0], CMD_NAME.c_str())) {
      handleError(UserError::INVALID_CMD_NAME);
      usage();
      std::exit(EXIT_FAILURE);
    }

    /* verify if a valid ipv4 address */
    std::string param(argv[1]);
    std::size_t pos = param.find("@");
    if (pos != std::string::npos) {
      username_ = param.substr(0, pos);
      std::string address = param.substr(pos + 1, param.size());
      struct sockaddr_in sa;
      int result = inet_pton(AF_INET, address.c_str(), &(sa.sin_addr));
      if (result == 1) {
        address_ = address;
      } else {
        handleError(UserError::INVALID_ADDR_STRING);
        usage();
        std::exit(EXIT_FAILURE);
      }
    } else {
      handleError(UserError::ILLFORMED_ARG);
      usage();
      std::exit(EXIT_FAILURE);
    }
  }

  std::string getUserName()
  {
    return username_;
  }

  std::string getAddress()
  {
    return address_;
  }

  bool isValid()
  {
    return isValid_;
  }
private:
  bool isValid_;
  std::string username_;
  std::string address_;
  const int MIN_ARG_CNT = 2;
  const std::string CMD_NAME = "./ssh-reverse-copy-id";
};

/* to hold the file info and stream info */
struct FtpFile {
  std::string filename;
  FILE* stream;
};

class SshReverseCopyId {
public:
  SshReverseCopyId(std::string username, std::string ipAddr)
    : username_(username), ipAddr_(ipAddr)
  {}

  static size_t flushToStagedTmp(void *buffer, size_t size, size_t nmemb,
                                 void *stream)
  {
    struct FtpFile *out = (struct FtpFile *)stream;
    if (!out->stream) {
      out->stream = fopen(out->filename.c_str(), "wb");
      if (!out->stream) {
        return -1;
      }
    }
    return fwrite(buffer, size, nmemb, out->stream);
  }
	
  /* terminal prompt to get confidential user input 
		 Note to self: termios works onl with tty like 
		 terminals.
	*/
	std::string passwdPrompt(std::string prompt)
	{
		std::cout << prompt;

		termios oldt;
		tcgetattr(STDIN_FILENO, &oldt);
		termios newt = oldt;
		newt.c_lflag &= ~ECHO;
		tcsetattr(STDIN_FILENO, TCSANOW, &newt);
		
		std::string tmp_passwd;
		std::getline(std::cin, tmp_passwd);

		newt.c_lflag |= ECHO;
		tcsetattr(STDIN_FILENO, TCSANOW, &newt);
		std::cout << std::endl;

		return tmp_passwd;
	}


  void execute()
  {
    CURL* curl;
    CURLcode res;
    FtpFile ftpFile = { "test.txt", nullptr };
    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if (curl) {
      std::string url = "sftp://" + username_ + ":raghu" + "@" + ipAddr_ + "/home/" + username_ + "/.ssh/id_rsa.pub";
      std::cout << "url: " << url << std::endl;
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, flushToStagedTmp);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpFile);
      
      curl_easy_setopt(curl, CURLOPT_USERNAME, username_.c_str());
      curl_easy_setopt(curl, CURLOPT_PASSWORD, passwdPrompt("passwd: ").c_str());
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    #ifndef DISABLE_SSH_AGENT
      curl_easy_setopt(curl, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_AGENT);
    #endif
      
      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      res = curl_easy_perform(curl);

      curl_easy_cleanup(curl);
      
      if (CURLE_OK != res) {
        fprintf(stderr, "Traceback::Internal Error::CURL : %d\n", res);
      }

      if (ftpFile.stream)
        fclose(ftpFile.stream);

      curl_global_cleanup();
    }
    
  }
private:
  std::string username_;
  std::string ipAddr_;
};

}

#endif // __SSH-REVERSE-COPY-ID_H__
