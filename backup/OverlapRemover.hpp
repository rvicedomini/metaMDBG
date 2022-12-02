

#ifndef MDBG_METAG_OverlapRemover
#define MDBG_METAG_OverlapRemover

#include "../Commons.hpp"




class OverlapRemover {
  

public:

	string _inputDir;
	string _inputFilenameContig;
	size_t _kminmerSize;
	int _nbCores;
	ofstream& _logFile;

	OverlapRemover(const string& inputDir, const string& inputFilenameContig, size_t kminmerSize, ofstream& logFile, int nbCores) : _logFile(logFile){
		_inputDir = inputDir;
		_inputFilenameContig = inputFilenameContig;
		_kminmerSize = kminmerSize-1;
		_nbCores = nbCores;
	}

	struct Contig{
		u_int32_t _contigIndex;
		vector<u_int64_t> _minimizers;
		vector<u_int32_t> _kminmers;
		bool isCircular;
	};

	static bool ContigComparator_ByLength(const Contig &a, const Contig &b){

		if(a._minimizers.size() == b._minimizers.size()){
			for(size_t i=0; i<a._minimizers.size() && i<b._minimizers.size(); i++){
				if(a._minimizers[i] == b._minimizers[i]){
					continue;
				}
				else{
					return a._minimizers[i] > b._minimizers[i];
				}
			}
		}


		return a._minimizers.size() < b._minimizers.size();
	}




	vector<Contig> _contigs;
	//typedef u_int32_t KminmerIndex;

	//struct KminmerIndex{
	//	u_int32_t _contigIndex;
		//u_int32_t _pos;
	//};

	typedef phmap::parallel_flat_hash_map<KmerVec, u_int32_t, phmap::priv::hash_default_hash<KmerVec>, phmap::priv::hash_default_eq<KmerVec>, std::allocator<std::pair<KmerVec, u_int32_t>>, 4, std::mutex> KminmerIndexMapLala;
	typedef phmap::parallel_flat_hash_map<u_int32_t, vector<u_int32_t>, phmap::priv::hash_default_hash<u_int32_t>, phmap::priv::hash_default_eq<u_int32_t>, std::allocator<std::pair<u_int32_t, vector<u_int32_t>>>, 4, std::mutex> KminmerReadIndexMap;


	//unordered_map<u_int32_t, vector<KminmerIndex>> _kminmerIndex;
	KminmerReadIndexMap _kminmerIndex;
	phmap::parallel_flat_hash_map<u_int32_t, u_int32_t> _contigLenths;

	static bool KminmerIndexComparator(const u_int32_t &a, const u_int32_t &b){
		return a < b;
	}


	void execute(){

		indexKminmers();
		indexContigs();
		_isModification = false;

		while(true){
			removeOverlaps();

			/*
			for(size_t i=0; i<_contigs.size(); i++){
				if(_contigs[i]._minimizers.size() == 0) continue;
				for(u_int64_t m : _contigs[i]._minimizers){
					_logFile << m << " ";
				}
				_logFile << endl;
				//_logFile << _contigs[i]._contigIndex << " " << _contigs[i]._minimizers.size() << endl;
			}
			*/

			//exit(1);




			//getchar();


			if(!_isModification) break;

			ofstream outputFile(_inputFilenameContig);
			u_int64_t nbContigs = 0;

			for(size_t i=0; i<_contigs.size(); i++){
				if(_contigs[i]._minimizers.size() == 0) continue;
				
				u_int32_t contigSize = _contigs[i]._minimizers.size();
				outputFile.write((const char*)&contigSize, sizeof(contigSize));
				outputFile.write((const char*)&_contigs[i].isCircular, sizeof(_contigs[i].isCircular));
				outputFile.write((const char*)&_contigs[i]._minimizers[0], contigSize*sizeof(u_int64_t));

				//_logFile << contigSize << endl;
				nbContigs += 1;
			}
			outputFile.close();


			//vector<Contig> contigsTmp = _contigs;
			_contigs.clear();
			_kminmerIndex.clear();


			cout << "Nb contigs: " << nbContigs << endl;

			indexContigs();
			/*
			u_int32_t contigIndex = 0;

			for(size_t i=0; i<contigsTmp.size(); i++){
				if(contigsTmp[i]._minimizers.size() == 0) continue;

				vector<u_int64_t> minimizersPos; 
				vector<u_int64_t> rlePositions; 
				vector<KmerVec> kminmers; 
				vector<ReadKminmer> kminmersInfo;
				MDBG::getKminmers(-1, _kminmerSize, contigsTmp[i]._minimizers, minimizersPos, kminmers, kminmersInfo, rlePositions, 0, false);

				indexContigs_read(contigsTmp[i]._minimizers, kminmers, kminmersInfo, contigsTmp[i].isCircular, contigIndex);
				contigIndex += 1;
			}

			cout << "Nb contigs: " << contigIndex << endl;
			*/

			//getchar();
		}
		
		removeOverlapsSelf();

		ofstream outputFile(_inputFilenameContig);
		u_int64_t nbContigs = 0;

		for(size_t i=0; i<_contigs.size(); i++){
			if(_contigs[i]._minimizers.size() == 0) continue;
			
			u_int32_t contigSize = _contigs[i]._minimizers.size();
			outputFile.write((const char*)&contigSize, sizeof(contigSize));
			outputFile.write((const char*)&_contigs[i].isCircular, sizeof(_contigs[i].isCircular));
			outputFile.write((const char*)&_contigs[i]._minimizers[0], contigSize*sizeof(u_int64_t));

			//_logFile << contigSize << endl;
			nbContigs += 1;
		}
		outputFile.close();

		//fs::remove(_inputFilenameContig);
		//fs::rename(_inputFilenameContig + ".nooverlaps", _inputFilenameContig);

		_logFile << nbContigs << endl;
		//getchar();


		//_inputFilenameContig = _inputFilenameContig + ".nooverlaps";

		/*
		_edgeIndex = 0;
		indexEdges();
		indexContigs();
		detectOverlaps();

		ofstream outputFile(_inputFilenameContig + ".nooverlaps");
		
		for(size_t i=0; i<_overContigs.size(); i++){
			if(_overContigs[i]._nodepath.size() == 0) continue;
			
			u_int32_t contigSize = _overContigs[i]._minimizers.size();
			outputFile.write((const char*)&contigSize, sizeof(contigSize));
			outputFile.write((const char*)&_overContigs[i]._minimizers[0], contigSize*sizeof(u_int64_t));
		}
		outputFile.close();

		_edgesToIndex.clear();
		_overContigs.clear();

		_inputFilenameContig = _inputFilenameContig + ".nooverlaps";
		*/
	}

	KminmerIndexMapLala _kminmerToIndex;
	u_int32_t _kminmerID;

	void indexKminmers(){

		_logFile << "Indexing kminmers" << endl;

		_kminmerID = 0;
		/*
		KminmerParser parser(_inputFilenameContig, -1, _kminmerSize, false, false);
		auto fp = std::bind(&OverlapRemover::indexKminmers_read, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
		parser.parse(fp);
		*/
	
		KminmerParserParallel parser(_inputFilenameContig, -1, _kminmerSize, false, false, _nbCores);
		parser.parse(IndexKminmerFunctor(*this));

	}

	/*
	void indexKminmers_read(const vector<u_int64_t>& readMinimizers, const vector<KmerVec>& vecs, const vector<ReadKminmer>& kminmersInfos, bool isCircular, u_int32_t readIndex){
		
		cout << readIndex << endl;
		for(u_int32_t i=0; i<vecs.size(); i++){
			
			KmerVec vec = vecs[i];

			if(_kminmerToIndex.find(vec) == _kminmerToIndex.end()){
				_kminmerToIndex[vec] = _kminmerID;
				_kminmerID += 1;
			}
		}
	}
	*/

	class IndexKminmerFunctor {

		public:

		OverlapRemover& _graph;

		IndexKminmerFunctor(OverlapRemover& graph) : _graph(graph){
		}

		IndexKminmerFunctor(const IndexKminmerFunctor& copy) : _graph(copy._graph){
		}

		~IndexKminmerFunctor(){
		}

		void operator () (const KminmerList& kminmerList) {


			u_int64_t readIndex = kminmerList._readIndex;
			const vector<u_int64_t>& readMinimizers = kminmerList._readMinimizers;
			//const vector<KmerVec>& kminmers = kminmerList._kminmers;
			const vector<ReadKminmerComplete>& kminmersInfos = kminmerList._kminmersInfo;


			for(size_t i=0; i<kminmersInfos.size(); i++){

				const ReadKminmerComplete& kminmerInfo = kminmersInfos[i];
				const KmerVec& vec = kminmerInfo._vec;

				bool isNewKey = false;

				_graph._kminmerToIndex.lazy_emplace_l(vec, 
				[](KminmerIndexMapLala::value_type& v) { // key exist
				},           
				[&vec, &isNewKey](const KminmerIndexMapLala::constructor& ctor) { // key inserted
					
					
					isNewKey = true;


					u_int32_t nodeName = -1;

					ctor(vec, nodeName); 

				}); // construct value_type in place when key not present

				
				if(isNewKey){

					u_int32_t nodeName;

					#pragma omp critical
					{

						
						nodeName = _graph._kminmerID;
						_graph._kminmerID += 1;
						

					}

					auto set_value = [&nodeName](KminmerIndexMapLala::value_type& v) { v.second = nodeName; };
					_graph._kminmerToIndex.modify_if(vec, set_value);
				}
			}
		}

	};

	void indexContigs(){

		_logFile << "Indexing contigs" << endl;

		/*
		KminmerParser parser(_inputFilenameContig, -1, _kminmerSize, false, false);
		auto fp = std::bind(&OverlapRemover::indexContigs_read, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
		parser.parse(fp);

		//_kminmerToIndex.clear();
		*/

		KminmerParserParallel parser(_inputFilenameContig, -1, _kminmerSize, false, false, _nbCores);
		parser.parse(IndexContigFunctor(*this));
	}


	class IndexContigFunctor {

		public:

		OverlapRemover& _graph;

		IndexContigFunctor(OverlapRemover& graph) : _graph(graph){
		}

		IndexContigFunctor(const IndexContigFunctor& copy) : _graph(copy._graph){
		}

		~IndexContigFunctor(){
		}

		void operator () (const KminmerList& kminmerList) {


			unordered_set<u_int32_t> indexedKminmer;

			u_int64_t readIndex = kminmerList._readIndex;
			const vector<u_int64_t>& readMinimizers = kminmerList._readMinimizers;
			//const vector<KmerVec>& kminmers = kminmerList._kminmers;
			const vector<ReadKminmerComplete>& kminmersInfos = kminmerList._kminmersInfo;

			vector<u_int32_t> nodepath;

			
			for(size_t i=0; i<kminmersInfos.size(); i++){

				const ReadKminmerComplete& kminmerInfo = kminmersInfos[i];
				const KmerVec& vec = kminmerInfo._vec;
				u_int32_t kminmerID = _graph._kminmerToIndex[vec];

				nodepath.push_back(kminmerID);

				if(indexedKminmer.find(kminmerID) == indexedKminmer.end()){

					_graph._kminmerIndex.lazy_emplace_l(kminmerID, 
					[&readIndex](KminmerReadIndexMap::value_type& v) { // key exist
						v.second.push_back(readIndex);
					},           
					[&kminmerID, &readIndex](const KminmerReadIndexMap::constructor& ctor) { // key inserted
						
						vector<u_int32_t> indexes;
						indexes.push_back(readIndex);

						ctor(kminmerID, indexes); 

					}); // construct value_type in place when key not present


					//_kminmerIndex[kminmerID].push_back({readIndex});
					indexedKminmer.insert(kminmerID);
				}

			}
			

			#pragma omp critical(indexContigFunctor)
			{
				_graph._contigLenths[readIndex] = readMinimizers.size();
				_graph._contigs.push_back({readIndex, readMinimizers, nodepath, kminmerList._isCircular});
			}
		}

	};

	
	void indexContigs_read(const vector<u_int64_t>& readMinimizers, const vector<KmerVec>& vecs, const vector<ReadKminmer>& kminmersInfos, bool isCircular, u_int32_t readIndex){

		unordered_set<u_int32_t> indexedKminmer;
		vector<u_int32_t> nodepath;
		
		for(u_int32_t i=0; i<vecs.size(); i++){
			
			//const ReadKminmerComplete& kminmerInfo = kminmersInfos[i];
			KmerVec vec = vecs[i];
			u_int32_t kminmerID = _kminmerToIndex[vec];

			if(indexedKminmer.find(kminmerID) == indexedKminmer.end()){
				_kminmerIndex[kminmerID].push_back(readIndex);
				indexedKminmer.insert(kminmerID);
			}

			nodepath.push_back(kminmerID);
			
		}
		
		_contigs.push_back({readIndex, readMinimizers, nodepath, isCircular});
	}
	
	/*
    struct ReadWriter{
        u_int64_t _readIndex;
        vector<u_int64_t> _minimizers;
		vector<u_int8_t> _minimizerQualities;
        //u_int32_t _prevNodeIndex;
    };

    struct ReadWriter_Comparator {
        bool operator()(ReadWriter const& p1, ReadWriter const& p2){
            return p1._readIndex > p2._readIndex;
        }
    };

	priority_queue<ReadWriter, vector<ReadWriter> , ReadWriter_Comparator> _readWriterQueue;
	u_int64_t _nextReadIndexWriter;

	void processOverlap(){

		//#pragma omp critical(dataupdate)
		#pragma omp critical
		{
			_readWriterQueue.push({read._index, minimizers, minimizerQualities});
			//_logFile << _readWriterQueue.size() << " " << read._index << " " << _nextReadIndexWriter << endl;

			while(!_readWriterQueue.empty()){

				const ReadWriter& readWriter = _readWriterQueue.top();

				if(readWriter._readIndex == _nextReadIndexWriter){

					//_logFile << "Writing read: " << _nextReadIndexWriter << endl;
					u_int32_t size = readWriter._minimizers.size();
					_file_readData.write((const char*)&size, sizeof(size));

					bool isCircular = false;
					_file_readData.write((const char*)&isCircular, sizeof(isCircular));

					_file_readData.write((const char*)&readWriter._minimizers[0], size*sizeof(u_int64_t));
					//_file_readData.write((const char*)&readWriter._minimizerQualities[0], size*sizeof(u_int8_t));

					_readWriterQueue.pop();
					_nextReadIndexWriter += 1;
				}
				else{
					break;
				}
			}
			
			//_logFile << readIndex << endl;
			//_file_readData.write((const char*)&minimizerPosOffset[0], size*sizeof(u_int16_t));
		}

	}
	*/


	void removeOverlapsSelf(){

		//isContigRemoved.resize(_contigs.size(), false);
		_logFile << "Removing self overlaps: " << _contigs.size()<< endl;

		//while(true){

			
			//_logFile << "loop" << endl;

		//std::sort(_contigs.begin(), _contigs.end(), ContigComparator_ByLength);
		//bool isModification = false;


		#pragma omp parallel num_threads(_nbCores)
		{

			#pragma omp for
			for(long i=0; i<_contigs.size(); i++){

				Contig& contig = _contigs[i];
				//_logFile << "---------------------" << endl;
				//_logFile << i << " " << contig._minimizers.size() << endl;

				if(contig._minimizers.size() == 0) continue;

				long n = longestPrefixSuffix(contig)-1;

				if(n <= 0) continue;
				

				//_logFile << "lala1: " << n << endl;
				
				contig._kminmers.resize(contig._kminmers.size()-n);
				//_logFile << contig._minimizers.size() << endl;
				contig._minimizers.resize(contig._minimizers.size()-n);

				//_logFile << "lala2: " << longestPrefixSuffix(contig) << endl;
			}
		}

	}

	void removeOverlaps(){

		_nextReadIndexWriter = 0;
		
		//isContigRemoved.resize(_contigs.size(), false);
		_logFile << "detecting overlaps" << endl;

		//while(true){

			
			//_logFile << "loop" << endl;

		std::sort(_contigs.begin(), _contigs.end(), ContigComparator_ByLength);
		_isModification = false;


        RemoveOverlapsFunctor functor(*this);
        u_int64_t i = 0;

        #pragma omp parallel num_threads(1)
        {

            RemoveOverlapsFunctor functorSub(functor);
            SuperbubbleWriter sw;

			while(true){

                Contig nodeIndex;
                bool isEof = false;

                #pragma omp critical
                {
                    
                    if(i >= _contigs.size()){
                        isEof = true;
                    }
                    else{
                        //sw = _contigs[i];
                        //_logFile << nodeIndex << endl;
                    	sw = {i};
                    }

                    i += 1;
                }

                if(isEof) break;
                functorSub(sw);

			}

		}
		
	}

    struct SuperbubbleWriter{
        u_int64_t _readIndex;
		u_int64_t _overlapSizeLeft;
		u_int64_t _overlapSizeRight;
		//Contig _contig;
        //u_int32_t _unitigIndex_source;
        //vector<u_int32_t> _nodeIndexRemoved;
        //u_int32_t _prevNodeIndex;
    };


    struct SuperbubbleWriter_Comparator {
        bool operator()(SuperbubbleWriter const& p1, SuperbubbleWriter const& p2){
            return p1._readIndex > p2._readIndex;
        }
    };


	priority_queue<SuperbubbleWriter, vector<SuperbubbleWriter> , SuperbubbleWriter_Comparator> _readWriterQueue;
	u_int64_t _nextReadIndexWriter;

	class RemoveOverlapsFunctor {

		public:

		OverlapRemover& _parent;

		RemoveOverlapsFunctor(OverlapRemover& parent) : _parent(parent){
        }

		RemoveOverlapsFunctor(const RemoveOverlapsFunctor& copy) : _parent(copy._parent){
		}

		~RemoveOverlapsFunctor(){
		}

		void operator () (SuperbubbleWriter sw) {

			

			OverlapRemover::Contig& contig = _parent._contigs[sw._readIndex];

			sw._overlapSizeLeft = 0;
			sw._overlapSizeRight = 0;

			if(contig._minimizers.size() == 0){
                _parent.writeContigOverlap(sw);
				return;
			}
			if(contig._minimizers.size() > 1000){
                _parent.writeContigOverlap(sw);
				return;
			}



			//_logFile << "-------" << endl;
			//for(u_int32_t nodeName : contig._kminmers){
			//	_logFile << nodeName << " ";
			//}
			//_logFile << endl;
			//_logFile << contig._kminmers.size() << endl;

			//size_t contigSize = contig._minimizers.size();
			u_int64_t overlapSizeLeft = _parent.computeOverlapSize_left(contig);
			//_logFile << "done -----" << endl;

			
			u_int64_t overlapSizeRight = _parent.computeOverlapSize_right(contig);

			sw._overlapSizeLeft = overlapSizeLeft;
			sw._overlapSizeRight = overlapSizeRight;

			_parent.writeContigOverlap(sw);
		}
	};
	
	bool _isModification;
	
	void writeContigOverlap(SuperbubbleWriter swNext){
        #pragma omp critical(superbubble)
        {


            //getchar();
            _readWriterQueue.push(swNext);

            while(!_readWriterQueue.empty()){

                const SuperbubbleWriter& sw = _readWriterQueue.top();

                //_logFile << sw._readIndex << " " << _nextReadIndexWriter << endl;
                if(sw._readIndex == _nextReadIndexWriter){

					//cout << sw._readIndex << " " << sw._overlapSizeLeft << " " << sw._overlapSizeRight << endl;

					Contig& contig = _contigs[sw._readIndex];
					u_int64_t overlapSizeLeft = sw._overlapSizeLeft;
					u_int64_t overlapSizeRight = sw._overlapSizeRight;

					//u_int64_t overlapTotalMin = 0;
					if(overlapSizeLeft > 0){
						overlapSizeLeft += _kminmerSize -1;
						//overlapTotalMin += overlapSizeLeft + _kminmerSize -1;
					}
					if(overlapSizeRight > 0){
						overlapSizeRight += _kminmerSize -1;
						//overlapTotalMin += overlapSizeRight + _kminmerSize -1;
					}

					if(overlapSizeLeft + overlapSizeRight == 0){
						//continue;
					}
					else if(overlapSizeLeft + overlapSizeRight >= contig._kminmers.size()){//} || overlapTotalMin >= contig._minimizers.size()){

						//_logFile << "remove total" << endl;

						_isModification = true;
						for(size_t i=0; i<contig._kminmers.size(); i++){
							
							for(u_int32_t& mIndex : _kminmerIndex[contig._kminmers[i]]){
								if(mIndex == contig._contigIndex){//} && mIndex._pos == i){
									//_logFile << "Removed: " << mIndex._contigIndex << " " << mIndex._pos << endl;
									mIndex = -1;
									//mIndex._pos = -1;
									break;
								}
							}
						}
						contig._minimizers.clear();
						contig._kminmers.clear();
					}
					else{
						
						//_logFile << "remove left and right" << endl;

						_isModification = true;
						for(size_t i=0; i<overlapSizeLeft; i++){
							
							for(u_int32_t& mIndex : _kminmerIndex[contig._kminmers[i]]){
								if(mIndex == contig._contigIndex){//} && mIndex._pos == i){
									//_logFile << "Removed: " << mIndex._contigIndex << " " << mIndex._pos << endl;
									mIndex = -1;
									//mIndex._pos = -1;
									break;
								}
							}
						}


						for(size_t i=0; i<overlapSizeRight; i++){
							size_t ii = contig._kminmers.size()-1-i;
							
							//_logFile << contig._minimizers.size()-1-i << " " << m << endl;
							for(u_int32_t& mIndex : _kminmerIndex[contig._kminmers[ii]]){
								if(mIndex == contig._contigIndex){//} && mIndex._pos == i){
									//_logFile << "Removed: " << mIndex._contigIndex << " " << mIndex._pos << endl;
									mIndex = -1;
									//mIndex._pos = -1;
									break;
								}
							}
						}

						

						if(overlapSizeLeft > 0){
							//overlapSizeLeft += (_kminmerSize-1);
							contig._kminmers.erase(contig._kminmers.begin(), contig._kminmers.begin() + overlapSizeLeft);
							contig._minimizers.erase(contig._minimizers.begin(), contig._minimizers.begin() + overlapSizeLeft);
						}
						if(overlapSizeRight > 0){
							//overlapSizeRight += (_kminmerSize-1);
							contig._kminmers.resize(contig._kminmers.size()-overlapSizeRight);
							//_logFile << contig._minimizers.size() << endl;
							contig._minimizers.resize(contig._minimizers.size()-overlapSizeRight);
							//_logFile << contig._minimizers.size() << endl;
							//contig._minimizers.erase(contig._minimizers.begin()+contig._minimizers.size()-1-overlapSizeRight, contig._minimizers.begin()+contig._minimizers.size()-1);
						}
					}



					if(contig._minimizers.size() <= _kminmerSize+1){ //+1
						_isModification = true;
						contig._minimizers.clear();
						contig._kminmers.clear();
					}
				


                    _readWriterQueue.pop();
                    _nextReadIndexWriter += 1;
                }
                else{
                    break;
                }
            }

        }
    }

		


	u_int32_t computeOverlapSize_left(const Contig& contig){

		vector<u_int32_t> currentContigIndex;
		u_int64_t overlapSize = 0;

		for(size_t p=0; p<contig._kminmers.size(); p++){

			vector<u_int32_t> nextContigIndex;

			for(const u_int32_t& mIndex : _kminmerIndex[contig._kminmers[p]]){
				if(mIndex == -1) continue;
				if(mIndex == contig._contigIndex) continue;
				if(_contigLenths[mIndex] < contig._minimizers.size()) continue;

				if(p == 0){
					//validContigIndex.insert(mIndex._contigIndex);
					currentContigIndex.push_back(mIndex);
				}
				else{
					nextContigIndex.push_back(mIndex);
				}
			}

			//_logFile << "pos: " << p << endl;

			
			if(p > 0){
				vector<u_int32_t> sharedContigIndexValid;

				std::sort(currentContigIndex.begin(), currentContigIndex.end(), KminmerIndexComparator);
				std::sort(nextContigIndex.begin(), nextContigIndex.end(), KminmerIndexComparator);

				//for(const MinimizerIndex& mIndex : currentContigIndex){
				//	_logFile << mIndex._contigIndex << " " << mIndex._pos << endl;
				//}
				//for(const MinimizerIndex& mIndex : nextContigIndex){
				//	_logFile << mIndex._contigIndex << " " << mIndex._pos << endl;
				//}

				size_t i=0;
				size_t j=0;
				while(i < currentContigIndex.size() && j < nextContigIndex.size()){

					//_logFile << p << " " << currentContigIndex[i]._contigIndex << " " << nextContigIndex[j]._contigIndex << endl;

					if(currentContigIndex[i] == nextContigIndex[j]){

						//if(contig._minimizers.size() > 1000){
						//	_logFile << p << " " << nextContigIndex[j]._contigIndex << endl;
						//}

						sharedContigIndexValid.push_back(nextContigIndex[j]);
						//if(nextContigIndex[j]._pos == currentContigIndex[i]._pos+1){
						//	sharedContigIndexValid.push_back(nextContigIndex[j]);
						//}

						i += 1;
						j += 1;
					}
					else if(currentContigIndex[i] < nextContigIndex[j]){
						i += 1;
					}
					else{
						j += 1;
					}

				}

				currentContigIndex = sharedContigIndexValid;
			}

			if(currentContigIndex.size() == 0) break;
			overlapSize += 1;
		}

		return overlapSize;
	}

	u_int32_t computeOverlapSize_right(const Contig& contig){

		long firstPos = contig._kminmers.size()-1;
		vector<u_int32_t> currentContigIndex;
		u_int64_t overlapSize = 0;

		for(long p=contig._kminmers.size()-1; p>=0; p--){

			vector<u_int32_t> nextContigIndex;

			for(const u_int32_t& mIndex : _kminmerIndex[contig._kminmers[p]]){
				if(mIndex == contig._contigIndex) continue;
				if(mIndex == -1) continue;
				if(_contigLenths[mIndex] < contig._minimizers.size()) continue;

				if(p == firstPos){
					//validContigIndex.insert(mIndex._contigIndex);
					currentContigIndex.push_back(mIndex);
				}
				else{
					nextContigIndex.push_back(mIndex);
				}
			}

			//_logFile << "pos: " << p << endl;

			
			if(p < firstPos){
				vector<u_int32_t> sharedContigIndexValid;

				std::sort(currentContigIndex.begin(), currentContigIndex.end(), KminmerIndexComparator);
				std::sort(nextContigIndex.begin(), nextContigIndex.end(), KminmerIndexComparator);

				//for(const MinimizerIndex& mIndex : currentContigIndex){
				//	_logFile << mIndex._contigIndex << " " << mIndex._pos << endl;
				//}
				//for(const MinimizerIndex& mIndex : nextContigIndex){
				//	_logFile << mIndex._contigIndex << " " << mIndex._pos << endl;
				//}

				size_t i=0;
				size_t j=0;
				while(i < currentContigIndex.size() && j < nextContigIndex.size()){

					//_logFile << p << " " << currentContigIndex[i]._contigIndex << " " << nextContigIndex[j]._contigIndex << endl;

					if(currentContigIndex[i] == nextContigIndex[j]){

						sharedContigIndexValid.push_back(nextContigIndex[j]);
						//if(nextContigIndex[j]._pos == currentContigIndex[i]._pos+1){
						//	sharedContigIndexValid.push_back(nextContigIndex[j]);
						//}

						i += 1;
						j += 1;
					}
					else if(currentContigIndex[i] < nextContigIndex[j]){
						i += 1;
					}
					else{
						j += 1;
					}

				}

				currentContigIndex = sharedContigIndexValid;
			}

			if(currentContigIndex.size() == 0) break;
			overlapSize += 1;
		}

		return overlapSize;
	}


	long longestPrefixSuffix(const Contig& contig)
	{
		int n = contig._minimizers.size();
	
		int lps[n];
		lps[0] = 0; // lps[0] is always 0
	
		// length of the previous
		// longest prefix suffix
		int len = 0;
	
		// the loop calculates lps[i]
		// for i = 1 to n-1
		int i = 1;
		while (i < n)
		{
			if (contig._minimizers[i] == contig._minimizers[len])
			{
				len++;
				lps[i] = len;
				i++;
			}
			else // (pat[i] != pat[len])
			{
				// This is tricky. Consider
				// the example. AAACAAAA
				// and i = 7. The idea is
				// similar to search step.
				if (len != 0)
				{
					len = lps[len-1];
	
					// Also, note that we do
					// not increment i here
				}
				else // if (len == 0)
				{
					lps[i] = 0;
					i++;
				}
			}
		}
	
		int res = lps[n-1];
	
		// Since we are looking for
		// non overlapping parts.
		return (res > n/2)? res/2 : res;
	}

};	


#endif 


