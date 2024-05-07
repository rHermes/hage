FROM gcc:13
LABEL authors="rhermes"

SHELL ["/bin/bash", "-exo", "pipefail", "-c"]

ENV DEBIAN_FRONTEND=noninteractive \
    TERM=dumb \
    PAGER=cat

# Configure environment
RUN echo 'APT::Get::Assume-Yes "true";' > /etc/apt/apt.conf.d/90circleci && \
	echo 'DPkg::Options "--force-confnew";' >> /etc/apt/apt.conf.d/90circleci && \
	apt-get update && apt-get install -y \
		curl \
		locales \
		sudo \
	&& \
    sed -i 's/^# *\(en_US.UTF-8\)/\1/' /etc/locale.gen && \
	locale-gen en_US.UTF-8 && \
	rm -rf /var/lib/apt/lists/* && \
	\
	groupadd --gid=1002 circleci && \
	useradd --uid=1001 --gid=circleci --create-home circleci && \
	echo 'circleci ALL=NOPASSWD: ALL' >> /etc/sudoers.d/50-circleci && \
	echo 'Defaults    env_keep += "DEBIAN_FRONTEND"' >> /etc/sudoers.d/env_keep && \
	sudo -u circleci mkdir /home/circleci/project && \
	sudo -u circleci mkdir /home/circleci/bin && \
	sudo -u circleci mkdir -p /home/circleci/.local/bin && \
	\
	dockerizeArch=arm64 && \
	if uname -m | grep "x86_64"; then \
		dockerizeArch=x86_64; \
	fi && \
	curl -sSL --fail --retry 3 --output /usr/local/bin/dockerize "https://github.com/powerman/dockerize/releases/download/v0.8.0/dockerize-linux-${dockerizeArch}" && \
	chmod +x /usr/local/bin/dockerize && \
	dockerize --version

ENV PATH=/home/circleci/bin:/home/circleci/.local/bin:$PATH \
	LANG=en_US.UTF-8 \
	LANGUAGE=en_US:en \
	LC_ALL=en_US.UTF-8

RUN noInstallRecommends="" && \
	if [[ "22.04" == "22.04" ]]; then \
		noInstallRecommends="--no-install-recommends"; \
	fi && \
    apt-get update && apt-get install -y $noInstallRecommends \
		autoconf \
		build-essential \
		ca-certificates \
		cmake \
		# already installed but here for consistency
		curl \
		file \
		gettext-base \
		gnupg \
		gzip \
		jq \
		libssl-dev \
		libsqlite3-dev \
		lsof \
		make \
		# compiling tool
		pkg-config \
		# already installed but here for consistency
		sudo \
		tar \
		tzdata \
		unzip \
        ninja-build cmake && \
	git version

USER circleci
# Run commands and tests as circleci user
RUN whoami && \
	# opt-out of the new security feature, not needed in a CI environment
	git config --global --add safe.directory '*'

# Match the default CircleCI working directory
WORKDIR /home/circleci/project