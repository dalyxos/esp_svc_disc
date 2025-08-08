FROM alpine:latest

# Install Avahi and dependencies
RUN apk add --no-cache \
    avahi \
    avahi-tools \
    dbus \
    && rm -rf /var/cache/apk/*

# Create necessary directories
RUN mkdir -p /var/run/dbus /var/run/avahi-daemon /etc/avahi/services

# Copy configuration files
COPY avahi-daemon.conf /etc/avahi/avahi-daemon.conf
COPY services/ /etc/avahi/services/

# Set proper permissions (try to set ownership, ignore errors if user doesn't exist)
RUN chown -R root:root /var/run/avahi-daemon || true

# Expose mDNS port
EXPOSE 5353/udp

# Start script
COPY start.sh /start.sh
RUN chmod +x /start.sh

CMD ["/start.sh"]
