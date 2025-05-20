#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

MAX_SIZE_GB=256  # maximun volume
ALLOW_LARGE_DISKS=false  # do not allow large disk operation

# confirm function
confirm_action() {
    local message=$1
    echo -e "${YELLOW}$message${NC}"
    read -p "Continue? [y/n]: " confirmation
    [[ "$confirmation" == "y" ]]
}

check_disk_size() {
    local disk=$1
    local size_bytes=$(lsblk -bno SIZE "$disk" | head -1)
    local size_gb=$((size_bytes / 1024 / 1024 / 1024))

    if [ "$ALLOW_LARGE_DISKS" = false ] && [ "$size_gb" -gt "$MAX_SIZE_GB" ]; then
        echo -e "${RED}Fail: The disk size is ${size_gb}GB, which exceeds (${MAX_SIZE_GB}GB)${NC}"
        echo -e "${BLUE}Info: If you still want to do it, please use --allow-large-disks parameter${NC}"
        return 1
    fi
    return 0
}

list_usb_devices() {
    echo -e "\n${GREEN}Available Devices:${NC}"
    echo "----------------------------------------"

    local counter=1
    declare -gA usb_map

    while read -r line; do
        if [[ -n "$line" ]]; then
            echo -e "[${counter}] ${GREEN}$line${NC}"
            disk_id=$(echo "$line" | awk '{print $1}')
            usb_map["$counter"]="/dev/$disk_id"
            ((counter++))
        fi
    done < <(lsblk -o NAME,SIZE,MODEL,VENDOR,MOUNTPOINT -d | \
             grep -v "NAME" | \
             grep -v "loop" | \
             grep -v "sr" | \
             grep -v "rom" | \
             while read -r device; do
                 dev_name=$(echo "$device" | awk '{print $1}')
                 udev_info=$(udevadm info --query=property --name=/dev/$dev_name)
                 if echo "$udev_info" | grep -q "ID_BUS=usb"; then
                     echo "$device"
                 fi
             done)

    echo "----------------------------------------"
    total_devices=$((counter-1))
}

get_partition_uuid() {
    local partition=$1

    local uuid=$(sudo blkid -o value -s UUID $partition 2>/dev/null)
    echo $uuid
    return 0
}

install() {
(
    local disk=$1
    local partition="${1}1"
    set -x
    local partition2="${disk}2"
    local uuid=$(get_partition_uuid $partition2)
    echo -e $partition
    sudo umount $disk* 2>/dev/null
    sudo apt install grub-efi-amd64-bin
    sudo wipefs -a $disk
    sudo parted $disk -- mklabel gpt
    sudo parted $disk -- mkpart ESP fat32 1MiB 300MiB
    sudo parted $disk -- set 1 esp on
    sudo mkfs.vfat -F32 $partition

    sudo mkdir -p /mnt/usbtmpyee
    sudo mount /dev/sda1 /mnt/usbtmpyee
    sudo mkdir -p /mnt/usbtmpyee/EFI/BOOT
    sudo mkdir -p /mnt/usbtmpyee/boot
    sudo grub-install \
        --target=x86_64-efi \
        --efi-directory=/mnt/usbtmpyee \
        --boot-directory=/mnt/usbtmpyee/boot \
        --removable \
        --force


    sudo mkinitramfs -k -o /mnt/usbtmpyee/boot/initrd.img $(uname -r)
    sudo cp /boot/vmlinuz-`uname -r` /mnt/usbtmpyee/boot/bzImage

    sudo parted $disk -- mkpart primary ext4 300MiB 100%
    local partition2="${disk}2"
    sudo mkfs.ext4 $partition2 -L rootfs

    uuid=$(get_partition_uuid $partition2)
    printf "set timeout=5\nset default=0\n\nmenuentry \"Live USB\" {\n    linux /boot/bzImage root=UUID=$uuid rootfstype=ext4 rw init=/init console=tty0\n    initrd /boot/initrd.img\n}\n" | sudo tee /mnt/usbtmpyee/boot/grub/grub.cfg > /dev/null
    sudo chmod 644 /mnt/usbtmpyee/boot/grub/grub.cfg


    sudo mkdir -p /mnt/rootfstmpyee
    sudo mount $partition2 /mnt/rootfstmpyee

    sudo mkdir -p /mnt/rootfs
    sudo mount /dev/sda2 /mnt/rootfs


    mkdir -p rootfs/{bin,dev,etc,proc,sys,usr/bin,usr/sbin,sbin}

    wget https://busybox.net/downloads/binaries/1.35.0-x86_64-linux-musl/busybox
    chmod +x busybox
    mv busybox rootfs/bin/

    cat > rootfs/etc/inittab <<EOF
::sysinit:/bin/busybox mount -t proc proc /proc
::sysinit:/bin/busybox mount -t sysfs sysfs /sys
::respawn:/bin/busybox sh
EOF

    cat > rootfs/etc/fstab <<EOF
proc /proc proc defaults 0 0
sysfs /sys sysfs defaults 0 0
EOF

    cat > rootfs/init <<EOF
#!/bin/busybox sh

/bin/busybox --install

mount -t devtmpfs devtmpfs /dev
mount -t proc proc /proc
mount -t sysfs sysfs /sys

export PATH=/bin:/sbin:/usr/bin:/usr/sbin

exec /bin/busybox sh
EOF
    chmod +x rootfs/init

    sudo cp -a rootfs/* /mnt/rootfstmpyee/


    sudo umount /mnt/usbtmpyee
    sudo rm -rf /mnt/usbtmpyee
    sudo umount /mnt/rootfstmpyee
    sudo rm -rf /mnt/rootfstmpyee
)
}

operate_on_usb() {
    local disk=$1


    if ! check_disk_size "$disk"; then
        return
    fi

    echo -e "\n${GREEN}Selected Disk: ${RED}$disk${NC}"
    echo -e "${YELLOW}Notice: some of the following operations will delete all the data permanently${NC}"
    echo "Operations:"
    echo "1. Install"
    echo "2. Exit"

    read -p "Choose Operation (1-2): " operation

    case $operation in
        1)
            if confirm_action "Are you sure to install live USB on $disk ?"; then
                echo -e "${YELLOW}Installing...${NC}"
                install $disk
                echo -e "${GREEN}Done${NC}"
            else
                echo -e "${YELLOW}Operation Cancelled${NC}"
            fi
            ;;
        2)
            echo "Exiting..."
            exit 0
            ;;
        *)
            echo -e "${RED}Invalid Operation${NC}"
            ;;
    esac
}

# parse command
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --allow-large-disks)
            ALLOW_LARGE_DISKS=true
            echo -e "${YELLOW}Warning: Enabled Large Disk Mode${NC}"
            shift
            ;;
        --max-size)
            MAX_SIZE_GB="$2"
            echo -e "${BLUE}Set the Max Disk Volume be ${MAX_SIZE_GB}GB${NC}"
            shift 2
            ;;
        *)
            echo -e "${RED}Unknown Parameters: $1${NC}"
            exit 1
            ;;
    esac
done

clear
echo -e "${GREEN}Bad Live USB Installer${NC}"
echo -e "${RED}Warning: This will potentially destroy your data, please use it carefully${NC}"
if [ "$ALLOW_LARGE_DISKS" = false ]; then
    echo -e "${BLUE}Safety Limit Enabled: You are not allowed to install it on the disk having volume more than ${MAX_SIZE_GB}GB${NC}"
    echo -e "${BLUE}Info: If you still want to do it, please use --allow-large-disks parameter${NC}"
else
    echo -e "${YELLOW}Waring: Large Volume Disk Operation Enabled${NC}"
fi
echo "========================================"

while true; do
    list_usb_devices

    if [[ "$total_devices" -eq 0 ]]; then
        echo -e "${YELLOW}USB Device Not Found. Please plug in USB Device and Retry${NC}"
        exit 1
    fi

    read -p "Choose USB Device (1-$total_devices) or Enter 'q'to Exit : " selection

    if [[ "$selection" == "q" ]]; then
        echo "Exiting..."
        exit 0
    elif [[ "$selection" -ge 1 && "$selection" -le "$total_devices" ]]; then
        selected_disk="${usb_map[$selection]}"
        echo -e "\n${GREEN}Chosen Device:${NC}"
        sudo lsblk -o NAME,SIZE,MODEL,VENDOR,FSTYPE,MOUNTPOINT "$selected_disk"

        operate_on_usb "$selected_disk"

        read -p "Press Enter to continue or 'q' to exit..." continue
        if [[ "$continue" == "q" ]]; then
            exit 0
        fi
        clear
    else
        echo -e "${RED}Invalid choice, please try again${NC}"
    fi
done