#!/bin/bash
set -e

REPO_OWNER="utcq"
REPO_NAME="wind"
INSTALL_DIR="/opt/wind/"

if [ -f "/usr/local/bin/windc" ]; then
  echo "windc compiler already exists in /usr/local/bin..."
  if [ -w "/usr/local/bin/" ]; then
    echo "Removing old windc compiler..."
    rm -f "/usr/local/bin/windc"
  else
    echo "Old windc compiler needs sudo to be removed..."
    sudo rm -f "/usr/local/bin/windc"
  fi
fi

echo "Fetching latest release for $REPO_OWNER/$REPO_NAME..."

LATEST_RELEASE=$(curl -s https://api.github.com/repos/$REPO_OWNER/$REPO_NAME/releases/latest | grep '"tag_name":' | sed -E 's/.*"([^"]+)".*/\1/')
RELEASE_VERSION=$(echo $LATEST_RELEASE | sed -E 's/Wind//')

if [ -z "$LATEST_RELEASE" ]; then
  echo "Failed to fetch the latest release. Please check your repository settings."
  exit 1
fi

echo "Latest release: $LATEST_RELEASE"

ASSET_URL="https://github.com/$REPO_OWNER/$REPO_NAME/releases/download/$LATEST_RELEASE/windc-dist-$RELEASE_VERSION.tar.gz"
TEMP_DIR=$(mktemp -d)

echo "Downloading from $ASSET_URL..."
curl -L -o "$TEMP_DIR/windc.tar.gz" "$ASSET_URL"

echo "Extracting archive..."
tar -xzf "$TEMP_DIR/windc.tar.gz" -C "$TEMP_DIR"

echo "Creating install directory..."
if [ -w "$(dirname "$INSTALL_DIR")" ]; then
  mkdir -p "$INSTALL_DIR"
else
  echo "Install directory ($INSTALL_DIR) needs sudo..."
  sudo mkdir -p "$INSTALL_DIR"
  sudo chown -R $(whoami) "$INSTALL_DIR"
fi

if [ -w "/usr/local/bin" ]; then
  ln -s "$INSTALL_DIR/windc" "/usr/local/bin/windc"
elif [ -w "/usr/bin" ]; then
  sudo ln -s "$INSTALL_DIR/windc" "/usr/bin/windc"
else
  echo "Binary needs sudo to be linked to /usr/local/bin..."
  sudo ln -s "$INSTALL_DIR/windc" "/usr/local/bin/windc"
fi

echo "Installing windc compiler..."
cp -r "$TEMP_DIR/wind-dist-$RELEASE_VERSION/"* "$INSTALL_DIR"
chmod +x "$INSTALL_DIR/windc"

echo "Cleaning up..."
rm -rf "$TEMP_DIR"

echo "windc compiler version $RELEASE_VERSION installed successfully!"
echo "You can now use it by typing 'windc' in your terminal."
