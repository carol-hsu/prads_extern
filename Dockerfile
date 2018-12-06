# cs7210
#
# Ke-Jou (Carol) Hsu // nosus_hsu@gatech.edu
#

FROM ubuntu:18.04

ARG DEFAULT_WORKDIR=/kube-nfx

# prerequisite installation
RUN apt-get update && \
    apt-get install -y gcc make git xutils-dev libevent-dev python-docutils libpcap0.8-dev libpcre3-dev iputils-ping

RUN mkdir $DEFAULT_WORKDIR
COPY ./ $DEFAULT_WORKDIR

WORKDIR $DEFAULT_WORKDIR/redis-lib
RUN make; exit 0
RUN make

WORKDIR $DEFAULT_WORKDIR
RUN make && make install
