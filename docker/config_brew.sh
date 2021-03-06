git clone https://github.com/Homebrew/homebrew.git /Volumes/Storage/goinfre/$(whoami)/homebrew
echo 'alias brew="/Volumes/Storage/goinfre/$(whoami)/homebrew/bin/brew"' >> ~/.zshrc
source ~/.zshrc
brew update
#brew tap watsonbox/cmu-sphinx
#brew install --HEAD watsonbox/cmu-sphinx/cmu-sphinxbase
#brew install --HEAD watsonbox/cmu-sphinx/cmu-pocketsphinx
#brew install --HEAD watsonbox/cmu-sphinx/cmu-sphinxtrain
brew install nvm
mkdir ~/.nvm
echo 'export NVM_DIR="$HOME/.nvm"' >> ~/.zshrc
echo 'source $(brew --prefix nvm)/nvm.sh' >> ~/.zshrc
source ~/.zshrc
nvm install 6.9.2
npm install -g npm@latest
echo 'Now checking the version ........ :'
node -v
npm -v
