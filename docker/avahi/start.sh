#!/bin/sh

# Create avahi user if it doesn't exist
if ! getent passwd avahi > /dev/null 2>&1; then
    addgroup -S avahi 2>/dev/null || true
    adduser -S -G avahi -s /bin/false avahi 2>/dev/null || true
fi

# Set proper permissions
chown -R avahi:avahi /var/run/avahi-daemon 2>/dev/null || chown -R root:root /var/run/avahi-daemon

# Start D-Bus daemon
mkdir -p /var/run/dbus
dbus-daemon --system --fork

# Wait a moment for D-Bus to start
sleep 2

# Start Avahi daemon
exec avahi-daemon --no-chroot --no-drop-root --debug
