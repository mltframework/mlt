@Remove_unnecessary_pointer_checks1@
expression x;
identifier release =~ "^(?x)
(?:cJSON_InitHooks
|  (?:mlt_)?free
|  mlt_(?:(?:animation
          |  cache(?:_item)?
          |  profile
          )_close
       |  consumer_purge
       |  properties_(?:un)?lock
       )
|  plugin_connect_output_ports
|  s(?:trbuf_close|et_mlt_normalisation)
)$";
@@
-if (\(x != 0 \| x != NULL\))
    release(x);

@Remove_unnecessary_pointer_checks2@
expression x;
identifier release =~ "^(?x)
(?:cJSON_InitHooks
|  (?:mlt_)?free
|  mlt_(?:(?:animation
          |  cache(?:_item)?
          |  profile
          )_close
       |  consumer_purge
       |  properties_(?:un)?lock
       )
|  plugin_connect_output_ports
|  s(?:trbuf_close|et_mlt_normalisation)
)$";
@@
-if (\(x != 0 \| x != NULL\)) {
    release(x);
    x = \(0 \| NULL\);
-}

@Remove_unnecessary_pointer_checks3@
expression a, b;
identifier release =~ "^(?x)
(?:cJSON_InitHooks
|  (?:mlt_)?free
|  mlt_(?:(?:animation
          |  cache(?:_item)?
          |  profile
          )_close
       |  consumer_purge
       |  properties_(?:un)?lock
       )
|  plugin_connect_output_ports
|  s(?:trbuf_close|et_mlt_normalisation)
)$";
@@
-if (\(a != 0 \| a != NULL\) && \(b != 0 \| b != NULL\))
+if (a)
    release(b);

@Remove_unnecessary_pointer_checks4@
expression a, b;
identifier release =~ "^(?x)
(?:cJSON_InitHooks
|  (?:mlt_)?free
|  mlt_(?:(?:animation
          |  cache(?:_item)?
          |  profile
          )_close
       |  consumer_purge
       |  properties_(?:un)?lock
       )
|  plugin_connect_output_ports
|  s(?:trbuf_close|et_mlt_normalisation)
)$";
@@
-if (\(a != 0 \| a != NULL\) && \(b != 0 \| b != NULL\)) {
+if (a) {
    release(b);
    b = \(0 \| NULL\);
 }
