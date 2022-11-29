FROM ghcr.io/toltec-dev/python:v1.4


RUN echo "Upgrading OKP to 0.53+"
# for rmkit
RUN pip3 install okp
