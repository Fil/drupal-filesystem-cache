<?php


/**
 * Return data from the persistent cache. Data may be stored as either plain text or as serialized data.
 * cache_get will automatically return unserialized objects and arrays.
 *
 * @param $cid
 *   The cache ID of the data to retrieve.
 * @param $table
 *   The table $table to store the data in. Valid core values are 'cache_filter',
 *   'cache_menu', 'cache_page', or 'cache' for the default cache.
 *
 *   @see cache_set()
 */
function cache_get($cid, $table = 'cache') {
  global $user;

	$f = cache_path($cid, $table);

	if (!@file_exists($f))
		return 0;

	$content = @unserialize(file_get_contents($f));

	if ($content['expire'] > 0
	AND $content['created'] + $content['expire'] < time())
		return 0;

	if ($content['cid'] <> $cid
	OR $content['table'] <> $table)
		return 0;

	return (object) $content;
}

/**
 * Store data in the persistent cache.
 *
 * The persistent cache is split up into four database
 * tables. Contributed modules can add additional tables.
 *
 * 'cache_page': This table stores generated pages for anonymous
 * users. This is the only table affected by the page cache setting on
 * the administrator panel.
 *
 * 'cache_menu': Stores the cachable part of the users' menus.
 *
 * 'cache_filter': Stores filtered pieces of content. This table is
 * periodically cleared of stale entries by cron.
 *
 * 'cache': Generic cache storage table.
 *
 * The reasons for having several tables are as follows:
 *
 * - smaller tables allow for faster selects and inserts
 * - we try to put fast changing cache items and rather static
 *   ones into different tables. The effect is that only the fast
 *   changing tables will need a lot of writes to disk. The more
 *   static tables will also be better cachable with MySQL's query cache
 *
 * @param $cid
 *   The cache ID of the data to store.
 * @param $data
 *   The data to store in the cache. Complex data types will be automatically serialized before insertion.
 *   Strings will be stored as plain text and not serialized.
 * @param $table
 *   The table $table to store the data in. Valid core values are 'cache_filter',
 *   'cache_menu', 'cache_page', or 'cache'.
 * @param $expire
 *   One of the following values:
 *   - CACHE_PERMANENT: Indicates that the item should never be removed unless
 *     explicitly told to using cache_clear_all() with a cache ID.
 *   - CACHE_TEMPORARY: Indicates that the item should be removed at the next
 *     general cache wipe.
 *   - A Unix timestamp: Indicates that the item should be kept at least until
 *     the given time, after which it behaves like CACHE_TEMPORARY.
 * @param $headers
 *   A string containing HTTP header information for cached pages.
 *
 *   @see cache_get()
 */
function cache_set($cid, $data, $table = 'cache', $expire = CACHE_PERMANENT, $headers = NULL) {
	$f = cache_path($cid, $table);

	$created = time();

	$content = array(
		'cid' => $cid,
		'data' => $data,
		'table' => $table,
		'expire' => $expire,
		'headers' => $headers,
		'created' => time()
	);

	($fp = @fopen($f.'.tmp', 'w'))
	&& @fwrite($fp, serialize($content))
	&& @fclose($fp)
	&& @rename($f.'.tmp', $f);
}

/**
 *
 * Expire data from the cache. If called without arguments, expirable
 * entries will be cleared from the cache_page and cache_block tables.
 *
 * @param $cid
 *   If set, the cache ID to delete. Otherwise, all cache entries that can
 *   expire are deleted.
 *
 * @param $table
 *   If set, the table $table to delete from. Mandatory
 *   argument if $cid is set.
 *
 * @param $wildcard
 *   If $wildcard is TRUE, cache IDs starting with $cid are deleted in
 *   addition to the exact cache ID specified by $cid.  If $wildcard is
 *   TRUE and $cid is '*' then the entire table $table is emptied.
 */
function cache_clear_all($cid = NULL, $table = NULL, $wildcard = FALSE) {
	$f = cache_path(null, null);
	foreach(glob(dirname($f).'/*/*.cache') as $del)
		@unlink($del);
}

function cache_path($cid, $table) {
	if (!is_dir($d = 'tmp')) {
		$d = sys_get_temp_dir().'/drupal';
		if (!is_dir($d)) @mkdir($d);
	}
	$dd = $d.'/'.substr(md5($table.$cid),0,2);
	if (!is_dir($dd)) @mkdir($dd);
	return $dd.'/'.substr(md5($table.$cid),2,2).'.cache';
}