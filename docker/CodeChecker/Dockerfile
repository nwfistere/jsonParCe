from alpine:latest

ENV PYTHONUNBUFFERED=1

RUN apk add --no-cache \
clang clang-extra-tools cppcheck g++ curl gcc git musl-dev linux-headers py3-pip python3-dev py3-setuptools cmake make bash \
  && ln -sf python3 /usr/bin/python

COPY . /src/

WORKDIR /src/

RUN python3 -m venv .venv
RUN . .venv/bin/activate && pip3 install codechecker setuptools

ENV CC=/usr/bin/gcc
ENV CXX=/usr/bin/g++
ENV PATH=$PATH:/src/.venv/bin

RUN echo "+/src/src/*" >> .skip_file
RUN echo "+/src/include*" >> .skip_file
RUN echo "-*" >> .skip_file
# "CodeChecker check --enable-all --build \"cmake -S . -B ./build && cmake --build ./build\" --ignore .skip_file"
# CMD [ "/bin/bash", "-c", "CodeChecker check --enable-all --build \"cmake -S . -B ./build && cmake --build ./build\" --ignore .skip_file" ]
CMD ["/bin/bash", "-c", "./docker/CodeChecker/entrypoint.sh"]