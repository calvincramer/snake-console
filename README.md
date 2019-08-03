# Snake using ANSI Escape Sequences

[ANSI](http://www.termsys.demon.co.uk/vtansi.htm) (American National Standards Institute) escape sequences allow control of the terminal just by printing specific strings. Unfortunately not all terminals support all features, so things like colors may appear differently on different terminals. I made this for Linux based systems with Docker and a Windows 10 host OS (using command prompt and [Cmder](https://cmder.net/)).

Credit to [sd9](https://www.linuxquestions.org/questions/programming-9/game-programming-non-blocking-key-input-740422/) for sections pertaining to changing the behavior of the standard input stream.

<br/>
<img src="https://media.giphy.com/media/U3U50jeSKRNpqzQ1kh/giphy.gif" width="640" height="576" />




# Installation and Compilation Instructions:

### 0. Clone this repository and nagivate into the folder

```
(navitage to an appropriate location first)
git clone https://github.com/calvincramer/snake_console.git snake
cd snake
```

### 1. Get the ubuntu base image: 

`docker pull ubuntu`

### 2. Create image from Dockerfile

`docker build -t snake_image .`

### 3. Create container with mounted folder

Without gdb:

`docker create --name=snake -it -v <path_to_snake_folder>:/snake snake_image /bin/bash`

With gdb support:

`docker create --cap-add=SYS_PTRACE --security-opt seccomp=unconfined --name=snake -it -v <path_to_snake_folder>:/snake snake_image /bin/bash`

Note that your mounted folder will be different. I did mine with a Windows host machine which is probably different than Linux. Also note you can mount multiple volumes just by adding more `-v` arguments.

For example, my `-v <path_to_snake_folder>:/snake` looked like this:

`-v //c/Users/CalvinLaptop/CalvinLaptop_Files/snake:/snake`

### 4. Start container and attach to container

```
docker start snake && docker attach snake
```

### 5. Navigate into snake folder and compile

```
cd /snake
```

```
make
./snake
```
or
```
make D=1
gdb snake
use gdb to your heart's content
```
