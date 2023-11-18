# Building the extension with docker

This directory contains an example showing you how to build and use the extension with the official postgres docker
image.

## Usage

```shell
docker compose up
```

After the container is started the extension is created automatically.

### Verify working extension

To verify that the extension is working correctly you can execute the following statment

```sql
SELECT roaringbitmap('{1,2,3}')
```