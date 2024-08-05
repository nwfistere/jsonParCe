ARG BASE_IMG=ubuntu:latest

FROM ${BASE_IMG} AS builder

RUN apt-get update && \
    apt-get install -y build-essential cmake git clang
