#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <unordered_map>

using SymbolID = uint32_t;
   
class IRuntimeValue {
public:
    virtual ~IRuntimeValue() = default;
    
};

class SomeValue : public IRuntimeValue {
public:
    int x{1142};
};

struct Val {
   std::string name;
   std::unique_ptr<IRuntimeValue> value;
   size_t hash{0};
};

class ValContext {

   std::vector<Val> value_;
   static constexpr int buckets = 256;
   std::hash<std::string> hasher;
   size_t cap;

public:
   
   ValContext() : cap{0} {
      value_.resize(buckets);  
   }

   bool isUsed(const Val& e) const {
      return !e.name.empty();
   }

   void re_hash(size_t newcap) {
 
      std::vector<Val> old_values = std::move(value_);

      value_ = std::vector<Val>(newcap);
      cap = 0;

      for (auto& e : old_values) {
         if (isUsed(e)) {
            size_t index = e.hash & (value_.capacity() - 1);
            while (isUsed(value_[index])) {
               index = (index + 1) & (value_.capacity() - 1);
            }

            value_[index] = std::move(e);
            cap++;
         }
      }
   }

   void insert(std::string name, std::unique_ptr<IRuntimeValue>&& value) {

      if (cap * 4 >= value_.capacity() * 3) {
         re_hash(cap * 2);
      }

      size_t hash = hasher(name);  
      size_t index = hash & (value_.capacity() - 1); // between 0 and 255

      while (isUsed(value_[index])) {
         if (value_[index].hash == hash && value_[index].name == name) {
            value_[index].value = std::move(value);
            return;
         }

         index = (index + 1) & (value_.capacity() - 1);

      }

      value_[index] = Val{std::move(name), std::move(value), hash};
      cap++;

   }
 
   IRuntimeValue * find(const std::string& name) const {

      size_t hash = hasher(name);  
      size_t index = hash & (value_.capacity() - 1);
      size_t start_index = index;

      while (isUsed(value_[index])) {
         if (value_[index].hash == hash && value_[index].name == name) {
            return value_[index].value.get();
         } 
         
         index = (index + 1) & (value_.capacity() - 1);

      }

      return nullptr;
   }

};

void test() {

   constexpr int ITERATIONS = 100'000;
   constexpr int NUM_FIELDS = 20;

   std::vector<std::string> field_names;
   
   for (int i = 0; i < NUM_FIELDS; ++i) {
      field_names.push_back("_field_" + std::to_string(i));
   }

   std::map<std::string, std::unique_ptr<IRuntimeValue>> map_obj;
   ValContext flat_obj;
   for (const auto& name : field_names) {
      map_obj[name] = std::make_unique<SomeValue>();
      flat_obj.insert(name, std::make_unique<SomeValue>());
   }

   std::cout << "--- BENCHMARK  (" << ITERATIONS << " object access) ---\n\n";

   // 1. std::map
   {
      auto start = std::chrono::high_resolution_clock::now();
      
      for (int iter = 0; iter < ITERATIONS; ++iter) {
         
         for (const auto& name : field_names) {
               void* ptr = map_obj[name].get();
               (void)ptr;
         }
      }

      auto end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double, std::milli> duration = end - start;
      std::cout << "std::map approach : " << duration.count() << " ms\n";
   }

   // 2. ValContext (Flat Hash Approach)
   {
      auto start = std::chrono::high_resolution_clock::now();
      
      for (int iter = 0; iter < ITERATIONS; ++iter) {
         
         for (const auto& name : field_names) {
               void* ptr = flat_obj.find(name);
               (void)ptr;
         }
      }

      auto end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double, std::milli> duration = end - start;
      std::cout << "Flat Hash Approach: " << duration.count() << " ms\n";
   }


}

int main() {

   std::map<std::string, std::unique_ptr<IRuntimeValue>> values_; 
   
   std::unordered_map<SymbolID, std::unique_ptr<IRuntimeValue>> myval; // causes allocation 1
     
   std::vector<std::string> str; // move strings to this vector
   std::vector<std::pair<SymbolID, std::unique_ptr<IRuntimeValue>>> val; // reserve 256 items 2
   
   std::vector<std::pair<std::string, std::unique_ptr<IRuntimeValue>>> vals; // reserve 256 items , move string into its location 3
   
   std::vector<Val> c; // hasher index approach & no string copy ?
   
   test();


}