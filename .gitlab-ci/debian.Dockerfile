FROM debian:trixie-slim

RUN export DEBIAN_FRONTEND=noninteractive \
   && apt-get -y update \
   && apt-get -y install --no-install-recommends wget ca-certificates gnupg eatmydata \
   && echo "deb http://deb.debian.org/debian-debug/ trixie-debug main" > /etc/apt/sources.list.d/debug.list \
   && eatmydata apt-get -y update \
   && eatmydata apt-get -y upgrade \
   && cd /home/user/app \
   && eatmydata apt-get --no-install-recommends -y build-dep . \
   && eatmydata apt-get --no-install-recommends -y install build-essential git wget gcovr locales uncrustify \
   && eatmydata apt-get --no-install-recommends -y install libglib2.0-0t64-dbgsym libgtk-4-1-dbgsym libadwaita-1-0-dbgsym libfontconfig1-dbgsym \
   && eatmydata apt-get clean

