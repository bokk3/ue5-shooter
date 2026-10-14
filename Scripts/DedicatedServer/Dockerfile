# Noble Fermi Dedicated Server Container
FROM ubuntu:22.04

# Install basic dependencies for UE5 server runtime
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    libstdc++6 \
    libgomp1 \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Create non-root dedicated server user
RUN useradd -m -s /bin/bash ue5server
WORKDIR /home/ue5server

COPY --chown=ue5server:ue5server . /home/ue5server/

USER ue5server

# Expose Game port and Steam Query port
EXPOSE 7777/udp
EXPOSE 27015/udp

RUN chmod +x ./start_server.sh

ENTRYPOINT ["./start_server.sh"]
