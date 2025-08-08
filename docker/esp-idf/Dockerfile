FROM espressif/idf:v5.1

# Install additional tools for testing
RUN apt-get update && apt-get install -y \
    avahi-utils \
    dnsutils \
    iputils-ping \
    telnet \
    curl \
    wget \
    vim \
    && rm -rf /var/lib/apt/lists/*

# Set up working directory
WORKDIR /workspace

# Copy entrypoint script
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
CMD ["/bin/bash"]
