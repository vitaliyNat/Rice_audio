#include <iostream>
#include <cmath>
#include "AudioFile.h"
#include <map>

using namespace std;

template <typename T>
class bitMask{
public:
    T mask;
    int size = sizeof(T);
    bitMask(){
        mask = 1;
    }

    void setLastBit(){
        mask = 1;
    }

    void setFirstBit(){
        mask = 1 << (size * 8 - 1);
    }
    void left(){
        mask = mask << 1;
        if(mask == 0){
            mask = 1;
        }
    }

    void right(){
        mask = mask >> 1;
        if(mask == 0){
            mask =  1 << (size * 8 - 1);
        }
    }


};

template <typename T>
vector<short> convertTo16bit(vector<T> samples){
    int size = samples.size();
    vector<short>  converted;
    converted.reserve(size);
    for(auto sample:samples){
        converted.push_back(round(sample * pow(2,15)));
    }
    return converted;
}

template <typename T>
vector<T> encodeDifferential(vector<T> samples){
    int size = samples.size();
    vector<T> encoded;
    encoded.reserve(size);;


    encoded.push_back(samples.front());
    for(int i = 1; i<size;i++){
        encoded.push_back(samples[i]-samples[i-1]);
    }


    return encoded;
}

template <typename T>
vector<T> decodeDifferential(vector<T> encodedSamples){
    int size = encodedSamples.size();
    vector<T> decoded;
    decoded.reserve(size);

    decoded.push_back(encodedSamples.front());
    for(int i = 1; i<size; i++){
        decoded.push_back(encodedSamples[i]+decoded[i-1]);
    }

    return decoded;
}

template <typename T>
bool testDecode(vector<T> original, vector<T> decoded){
    int size = original.size();
    if(size != decoded.size()){
        cout<<"Failed: decoded size not equals original size"<<endl;
        return false;
    }

    for(int i = 0; i<size; i++){
        if(original[i] !=decoded[i]){
            cout<<"Failed: Sample nr "<<i<<" in decoded data not equal to original"<<endl;
            return false;
        }
    }

    cout<<"Success: Decoded data equals original "<<endl;
    return true;
}


template <typename T>
map<T,double> findDistribution(vector<T> samples){
    int size = samples.size();

    map<T,double> distribution;

    for(auto sample : samples){
        distribution[sample]++;
    }

    for(auto &[key, value] : distribution){
        value = value/size;
    }

    return distribution;
}


template <typename T>
double zeroOrderEntropy(vector<T> samples){
    auto distribution = findDistribution(samples);

    double H = 0;

    for(auto &[key,value]:distribution){
        if(value !=0){
            H += value* log2(value);
        }
    }
    H = -H;
    distribution.clear();

    return H;

}

template <typename T>
vector<unsigned short> convertToUnsigned(vector<T> samples){
    int size = samples.size();
    vector<unsigned short> unsignedSamples;
    unsignedSamples.reserve(size);

    for(auto sample: samples){
        unsignedSamples.push_back(sample >=0 ? sample*2 : sample *(-2) -1);
    }
    return unsignedSamples;
}


vector<short> convertToSigned(vector<unsigned short>  unsignedSamples){
    unsigned int size = unsignedSamples.size();
    vector<short> signedSamples;
    signedSamples.reserve(size);

    for(auto sample:unsignedSamples){
        signedSamples.push_back((sample & 1) ? -(sample+1)/2 : sample/2);
    }

    return signedSamples;
}

template <typename T>
double findDistributionRice(vector<T> samples){
    long long sum = 0;
    for(auto sample:samples){
        sum+=sample;
    }
    double s = sum/samples.size();
    return (s-1)/s;
}

void writeBit(ofstream& file, bool bit){
    static unsigned char currentByte = 0;
    static char bitPos = 0;

    if(bit){
        currentByte |= (1 << bitPos);
    }

    bitPos++;
    if(bitPos == 8){
        file.put(currentByte);
        currentByte = 0;
        bitPos = 0;
    }
}

char kValueRice(double p){
    return ceil(log2((log2((sqrt(5)-1)/2))/log2(p)));
}

void writeUnary(ofstream& stream, unsigned int u){
    for(unsigned int i = 0; i<u;i++){
        writeBit(stream,false);
    }
    writeBit(stream,true);
}

void writeBinary(ofstream& stream, unsigned int v, unsigned int bits){
    unsigned int bitPos = (1<<bits);
    for(int i = 0; i < bits; i++){
        writeBit(stream, (bool)(bitPos & v));
        bitPos>>=1;
    }
}

template <typename T>
unsigned long long encodeRice(vector<T> samples, const string& name){
    ofstream outFile(".//datFiles//"+name+".dat", ios::binary);

    if (!outFile.is_open()) {
        cerr << "Error opening file!" << endl;
        return 0;
    }

    unsigned long long bitsLength = 0;
    auto k = kValueRice(findDistributionRice(samples));
    unsigned long long originalSize = samples.size();

    if(k>15) k = 15;
    outFile.write(reinterpret_cast<const char *> (&originalSize),sizeof(originalSize));
    outFile.put(k);

    auto twoPowk = pow(2,k);
    unsigned int u = 0;
    unsigned int v = 0;

    for(auto sample:samples){
        u = floor(sample/twoPowk);
        writeUnary(outFile,u);
        v = sample - u*twoPowk;
        writeBinary(outFile, v,k);
        bitsLength+=u+k+1;
    }
    char bitsNeeded =(char) (bitsLength%8);
    for(char i = 0; i<bitsNeeded; i++){
        writeBit(outFile,false);
    }
    outFile.close();

    return bitsLength;

}






int main() {

    AudioFile<double> audioFile;

    audioFile.load(".//audio//ATrain.wav");
    audioFile.printSummary();


    auto converted = convertTo16bit(audioFile.samples[0]);

    auto encoded = encodeDifferential(converted);
    auto decoded = decodeDifferential(encoded);

    auto test = testDecode(converted,decoded);

    cout<<endl<<zeroOrderEntropy(converted)<<endl;


    auto unsignedDifferentialSamples = convertToUnsigned(encoded);

    auto encodedRice = encodeRice(unsignedDifferentialSamples,"ATrain");

    cout<<encodedRice<<endl;



    return 0;
}