FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    avahi-utils \
    avahi-daemon \
    dnsutils \
    iputils-ping \
    telnet \
    curl \
    wget \
    python3 \
    python3-pip \
    nodejs \
    npm \
    vim \
    net-tools \
    && rm -rf /var/lib/apt/lists/*

# Install Python test dependencies
RUN pip3 install \
    pytest \
    pytest-asyncio \
    requests \
    zeroconf \
    python-multicast \
    pymodbus

# Copy test scripts
COPY test-runner.py /usr/local/bin/test-runner.py
COPY run-tests.sh /usr/local/bin/run-tests.sh
RUN chmod +x /usr/local/bin/test-runner.py /usr/local/bin/run-tests.sh

WORKDIR /workspace

CMD ["/bin/bash"]
