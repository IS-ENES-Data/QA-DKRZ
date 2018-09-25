#include "brace_op.h"

BraceOP::BraceOP()
{
  nextCounter=0;
}

BraceOP::BraceOP(std::string &s)
{
  str=s;
  nextCounter=0;
 }

BraceOP::BraceOP(std::vector<std::string> &s)
{
  set(s);
}

void
BraceOP::add(std::vector<std::string> &vs)
{
  if( nextCounter )
    str.clear();

  for(size_t i=0 ; i < vs.size() ; ++i )
    str += vs[i];

  nextCounter=0;

  return;
}

void
BraceOP::add(std::string s)
{
  if( nextCounter )
    str.clear();

  nextCounter=0;
  str += s;
  return;
}

void
BraceOP::clear(void)
{
  str.clear();
  return;
}

void
BraceOP::findBraces(void)
{
  // find { or } positions and corresponding levels.
  // due to nesting there might be multiple braces or
  // nested braces with a same level count.

  str = hdhC::clearSpaces( str );

  // split something like: ..{..,{{..}..},{..}..},{..}

  // Note: leftLevel points to the first char inside a brace
  //       rightLevel points to the last char in a brace

  // insert { before and } behind, respectively, to ensure a zero-th level
  std::string tmp("{");
  tmp += str ;
  tmp += '}';
  str = tmp;

  std::string s;
  int lC=0 ; // level counter
  size_t last=0;

  Branch tree;
  Branch *currB=&tree ;

  for( size_t pos=0 ; pos < str.size(); ++pos )
  {
    s.clear();

    if( str[pos] == '{' )
    {
      currB = currB->branch();

      if( ! ( (last +1 ) > pos) )
      {
        // brace is open right && non-brace item is given
        s = str.substr(last+1, pos - last - 1);

        if( s.size() && s != "," )
        {
          // item could be placed at a brace without comma
          size_t sz = currB->prev->str.size() ;

       	  if( s[0] != ',' )
            if( sz && currB->prev->str[sz-1] != ',' )
              currB->prev->str += ',';
          currB->prev->str += s;
        }
      }

      last=pos;
    }
    else if( str[pos] == '}' )
    {
      if( (last+1) != pos)
      {
        // close an open brace expr
        s = str.substr(last+1, pos - last -1) ;

       	if( ! (s.size() == 0 || s == ",") )
        {
          // item could be placed at a brace without comma
          // size_t sz = currB->prev ? currB->prev->str.size() : 0 ;
          size_t sz = currB->prev == nullptr ? 0 : currB->prev->str.size() ;

          if( s[0] != ',' )
            if( sz && currB->str[currB->str.size()-1] != ',' )
              currB->str += ',';
          currB->str += s ;
        }
      }

      last = pos;
      --lC;
      currB = currB->prev;
    }
  }

  // this scans recursively through the branches identifying the one higest one
  groups.clear();

  scanTree(tree);

  tree.deleteTree();

  return ;
}

void
BraceOP::getGroup(Branch *b_top)
{
   // The sub-tree could be temporarily changed, thus a copy.
   Branch bc;
   bc.cpTreeDownwards(b_top);

   Branch* b = &bc;

   // discard lower items by D(items) of higher level; top-down a tree
   rule_discard_lower(b);

   // combine levels; top -> down: top overrules
   rule_top_down(b);

   std::string str( hdhC::stripSides(b->str, ",") );

   while( b->prev != nullptr )
   {
       if( b->prev->str.size() )
          str = b->prev->str + "," + str ;

       b = b->prev;
   }

   str = hdhC::stripSides(str, ",") ;
   groups.push_back( hdhC::unique(str, ',') );

   return;
}

void
BraceOP::rule_top_down(Branch* b)
{
   if( b->prev == nullptr || b->str.size() == 0 )
       return;

   // for discarding outer elements: macro D(item)
   Branch* b_high = b;

   std::string high_key;
   std::string low_key;

   // note that the lower parts of a tree have to be searched
   // for each branch, because the original sub-tree remains untouched.
   while( b_high->prev != nullptr)
   {
      Split x_high(hdhC::clearSpaces(b_high->str), ",");

      for(size_t j=0 ; j < x_high.size() ; ++j)
      {
          size_t pos;
          if( (pos=x_high[j].find('=')) < std::string::npos )
              high_key=x_high[j].substr(0,pos);
          else
              high_key=x_high[j];

          // scan the lower sub-tree
          Branch* b_low = b_high->prev;
          std::string res_low;

          while( b_low->prev != nullptr )
          {
              std::string low_str=hdhC::clearSpaces(b_low->str);

              Split x_low(low_str, ',');
              res_low.clear();
              bool isLowChanged=false;

              for( size_t i=0 ; i < x_low.size() ; ++i )
              {
                 if( (pos=x_low[i].find('=')) < std::string::npos )
                    low_key=x_low[i].substr(0,pos);
                 else
                    low_key=x_low[i];

                 if( low_key != high_key )
                 {
                    if( res_low.size() )
                        res_low += "," ;
                    res_low += x_low[i] ;
                 }
                 else
                    isLowChanged=true;
              }

              if( isLowChanged )
                  b_low->str = res_low ;

              b_low = b_low->prev;
          }
      }

      b_high = b_high->prev ;
   }

   return;
}

void
BraceOP::rule_discard_lower(Branch* b)
{
   if( b->prev == nullptr || b->str.size() == 0 )
       return;

   // for discarding outer elements: macro D(item)
   Branch* b_high = b;

   std::string high_res;
   std::string high_str;

   // note that the lower parts of a tree have to be searched
   // for each branch, because the original sub-tree remains untouched.
   while( b_high->prev != nullptr)
   {
      size_t pos0=0;
      size_t pos1;
      size_t pos2;
      bool isHighChanged =false;
      high_str=hdhC::clearSpaces(b_high->str);
      high_res.clear();

      // D() could contain multiple items
      while( (pos1=high_str.find("D(", pos0)) < std::string::npos )
      {
         high_res += high_str.substr(pos0, pos1-pos0);

         isHighChanged=true;

         pos1 += 2;
         pos2 = b_high->str.find(")", pos1) ;
         std::string str_D(high_str.substr(pos1,pos2-pos1)) ;
         pos0 = pos2+1 ;

         Split x_hD(str_D, ",");

         for(size_t jD=0 ; jD < x_hD.size() ; ++jD)
         {
            // scan the lower sub-tree
            Branch* b_low = b_high->prev;
            std::string res_low;

            while( b_low->prev != nullptr )
            {
                std::string low_str=hdhC::clearSpaces(b_low->str);

                if( b_low->str.size() )
                {
                   Split x_low(low_str, ',');
                   res_low.clear();
                   bool isLowChanged=false;

                   for( size_t i=0 ; i < x_low.size() ; ++i )
                   {
                       if( x_low[i] != x_hD[jD] )
                       {
                          if( res_low.size() )
                              res_low += "," ;
                          res_low += x_low[i] ;
                       }
                       else
                          isLowChanged=true;
                   }

                   if( isLowChanged )
                       b_low->str = res_low ;
                }

                b_low = b_low->prev;
            }
         }
      }


      if( isHighChanged )
      {
         if( pos0 < high_str.size() )
            high_res += high_str.substr(pos0) ;

         b_high->str = high_res ;
      }

      b_high = b_high->prev ;
   }

   return;
}

void
BraceOP::scanTree(Branch &b)
{
   // reaching the tip of this sub-tree
   if( b.str.size() > 0 )
   {
       // assemble group
       getGroup(&b);

       if( b.next.size() == 0)
          return ;
   }

   for( size_t i=0 ; i < b.next.size() ; ++i )
       scanTree(*b.next[i]) ;

   return ;
}

bool
BraceOP::next(std::string &s)
{
   if( nextCounter == 0  )
      findBraces();

   if( nextCounter == groups.size() )
     return false;

   s = groups[nextCounter++];

   return true;
}

void
BraceOP::printGroups(void)
{
  for(size_t j=0 ; j < groups.size() ; ++j )
    std::cout << groups[j] << std::endl;

  return ;
}

void
BraceOP::set(std::vector<std::string> &vs)
{
  str.clear();
  for(size_t i=0 ; i < vs.size() ; ++i )
    str += vs[i];

  nextCounter=0;

  return;
}

void
BraceOP::set(std::string s)
{
  nextCounter=0;
  str = s;
  return;
}

// ----------------

void
Branch::cpTreeDownwards(Branch *p0)
{
  Branch* p = this;

  // destroy any tree build from this instance;
  // then the vector next is empty, but exists
  deleteTree();

  //while( p0 != nullptr )
  while( p0->prev )
  {
    p->str=p0->str;
    p->prev = new Branch() ;
    p->prev->next.push_back(this) ;

    p=p->prev;
    p0 = p0->prev;
  }

  return ;
}

Branch*
Branch::branch(void)
{
  next.push_back(new Branch(this));
  return next.back();
}

void
Branch::deleteTree(void)
{
  for( size_t i=0 ; i < next.size() ; ++i )
  {
    next[i]->deleteTree();
    delete next[i];
  }

  return;
}
